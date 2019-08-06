#!/usr/bin/env python3

import argparse
import json
import sys
import unicodedata
import xml.etree.ElementTree as ET
from datetime import datetime


aparser = argparse.ArgumentParser(
    description='Convert Meson test log into JUnit report')
aparser.add_argument('--project-name', metavar='NAME',
                     help='The project name',
                     default='unknown')
aparser.add_argument('--job-id', metavar='ID',
                     help='The job ID for the report',
                     default='Unknown')
aparser.add_argument('--branch', metavar='NAME',
                     help='Branch of the project being tested',
                     default='master')
aparser.add_argument('--output', metavar='FILE',
                     help='The output file, stdout by default',
                     type=argparse.FileType('w', encoding='UTF-8'),
                     default=sys.stdout)
aparser.add_argument('infile', metavar='FILE',
                     help='The input testlog.json, stdin by default',
                     type=argparse.FileType('r', encoding='UTF-8'),
                     default=sys.stdin)
args = aparser.parse_args()

outfile = args.output

testsuites = ET.Element('testsuites')
testsuites.set('id', '{}/{}'.format(args.job_id, args.branch))
testsuites.set('package', args.project_name)
testsuites.set('timestamp', datetime.utcnow().isoformat(timespec='minutes'))

testsuite = ET.SubElement(testsuites, 'testsuite')
testsuite.set('name', args.project_name)

successes = 0
failures = 0
skips = 0


def escape_control_chars(text):
    return "".join(c if unicodedata.category(c)[0] != "C" else
                   "<{:02x}>".format(ord(c)) for c in text)


for line in args.infile:
    unit = json.loads(line)

    testcase = ET.SubElement(testsuite, 'testcase')
    testcase.set('classname', '{}/{}'.format(args.project_name, unit['name']))
    testcase.set('name', unit['name'])
    testcase.set('time', str(unit['duration']))

    stdout = escape_control_chars(unit.get('stdout', ''))
    stderr = escape_control_chars(unit.get('stderr', ''))
    if stdout:
        ET.SubElement(testcase, 'system-out').text = stdout
    if stderr:
        ET.SubElement(testcase, 'system-out').text = stderr

    result = unit['result'].lower()
    if result == 'skip':
        skips += 1
        ET.SubElement(testcase, 'skipped')
    elif result == 'fail':
        failures += 1
        failure = ET.SubElement(testcase, 'failure')
        failure.set('message', "{} failed".format(unit['name']))
        failure.text = "### stdout\n{}\n### stderr\n{}\n".format(stdout,
                                                                 stderr)
    else:
        successes += 1
        assert unit['returncode'] == 0

testsuite.set('tests', str(successes + failures + skips))
testsuite.set('skipped', str(skips))
testsuite.set('errors', str(failures))
testsuite.set('failures', str(failures))

print('{}: {} pass, {} fail, {} skip'.format(args.project_name,
                                             successes,
                                             failures,
                                             skips))

output = ET.tostring(testsuites, encoding='unicode')
outfile.write(output)
