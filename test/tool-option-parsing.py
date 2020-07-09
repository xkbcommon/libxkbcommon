#!/usr/bin/env python3
#
# Copyright © 2020 Red Hat, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import itertools
import os
import resource
import sys
import subprocess
import logging
import tempfile

try:
    import pytest
except ImportError:
    print('Failed to import pytest. Skipping.', file=sys.stderr)
    sys.exit(77)


top_builddir = os.environ['top_builddir']

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger('test')
logger.setLevel(logging.DEBUG)

# Permutation of RMLVO that we use in multiple tests
rmlvos = [list(x) for x in itertools.permutations(
    ['--rules=evdev', '--model=pc104',
     '--layout=fr', '--options=eurosign:5']
)]


def _disable_coredump():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


def run_command(args):
    logger.debug('run command: {}'.format(' '.join(args)))

    try:
        p = subprocess.run(args, preexec_fn=_disable_coredump,
                           capture_output=True, text=True,
                           timeout=0.7)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired as e:
        return 0, e.stdout, e.stderr


class XkbcliTool:
    xkbcli_tool = 'xkbcli'
    subtool = None

    def __init__(self, subtool=None):
        self.tool_path = top_builddir
        self.subtool = subtool

    def run_command(self, args):
        if self.subtool is not None:
            tool = '{}-{}'.format(self.xkbcli_tool, self.subtool)
        else:
            tool = self.xkbcli_tool
        args = [os.path.join(self.tool_path, tool)] + args

        return run_command(args)

    def run_command_success(self, args):
        rc, stdout, stderr = self.run_command(args)
        assert rc == 0, (stdout, stderr)
        return stdout, stderr

    def run_command_invalid(self, args):
        rc, stdout, stderr = self.run_command(args)
        assert rc == 2, (rc, stdout, stderr)
        return rc, stdout, stderr

    def run_command_unrecognized_option(self, args):
        rc, stdout, stderr = self.run_command(args)
        assert rc == 2, (rc, stdout, stderr)
        assert stdout.startswith('Usage') or stdout == ''
        assert 'unrecognized option' in stderr

    def run_command_missing_arg(self, args):
        rc, stdout, stderr = self.run_command(args)
        assert rc == 2, (rc, stdout, stderr)
        assert stdout.startswith('Usage') or stdout == ''
        assert 'requires an argument' in stderr


def get_tool(subtool=None):
    return XkbcliTool(subtool)


def get_all_tools():
    return [get_tool(x) for x in [None, 'list',
                                  'compile-keymap',
                                  'how-to-type',
                                  'interactive-evdev',
                                  'interactive-wayland',
                                  'interactive-x11']]


@pytest.fixture
def xkbcli():
    return get_tool()


@pytest.fixture
def xkbcli_list():
    return get_tool('list')


@pytest.fixture
def xkbcli_how_to_type():
    return get_tool('how-to-type')


@pytest.fixture
def xkbcli_compile_keymap():
    return get_tool('compile-keymap')


@pytest.fixture
def xkbcli_interactive_evdev():
    return get_tool('interactive-evdev')


@pytest.fixture
def xkbcli_interactive_x11():
    return get_tool('interactive-x11')


@pytest.fixture
def xkbcli_interactive_wayland():
    return get_tool('interactive-wayland')


# --help is supported by all tools
@pytest.mark.parametrize('tool', get_all_tools())
def test_help(tool):
    stdout, stderr = tool.run_command_success(['--help'])
    assert stdout.startswith('Usage:')
    assert stderr == ''


# --foobar generates "Usage:" for all tools
@pytest.mark.parametrize('tool', get_all_tools())
def test_invalid_option(tool):
    tool.run_command_unrecognized_option(['--foobar'])


# xkbcli --version
def test_xkbcli_version(xkbcli):
    stdout, stderr = xkbcli.run_command_success(['--version'])
    assert stdout.startswith('0')
    assert stderr == ''


def test_xkbcli_too_many_args(xkbcli):
    xkbcli.run_command_invalid(['a'] * 64)


@pytest.mark.parametrize('args', [['--verbose'],
                                  ['--rmlvo'],
                                  # ['--kccgst'],
                                  ['--verbose', '--rmlvo'],
                                  # ['--verbose', '--kccgst'],
                                  ])
def test_compile_keymap_args(xkbcli_compile_keymap, args):
    xkbcli_compile_keymap.run_command_success(args)


@pytest.mark.parametrize('rmlvos', rmlvos)
def test_compile_keymap_rmlvo(xkbcli_compile_keymap, rmlvos):
    xkbcli_compile_keymap.run_command_success(rmlvos)


@pytest.mark.parametrize('args', [['--include', '.', '--include-defaults'],
                                  ['--include', '/tmp', '--include-defaults'],
                                  ])
def test_compile_keymap_include(xkbcli_compile_keymap, args):
    # Succeeds thanks to include-defaults
    xkbcli_compile_keymap.run_command_success(args)


def test_compile_keymap_include_invalid(xkbcli_compile_keymap):
    # A non-directory is rejected by default
    args = ['--include', '/proc/version']
    rc, stdout, stderr = xkbcli_compile_keymap.run_command(args)
    assert rc == 1, (stdout, stderr)
    assert "There are no include paths to search" in stderr

    # A non-existing directory is rejected by default
    args = ['--include', '/tmp/does/not/exist']
    rc, stdout, stderr = xkbcli_compile_keymap.run_command(args)
    assert rc == 1, (stdout, stderr)
    assert "There are no include paths to search" in stderr

    # Valid dir, but missing files
    args = ['--include', '/tmp']
    rc, stdout, stderr = xkbcli_compile_keymap.run_command(args)
    assert rc == 1, (stdout, stderr)
    assert "Couldn't look up rules" in stderr


# Unicode codepoint conversions, we support whatever strtol does
@pytest.mark.parametrize('args', [['123'], ['0x123'], ['0123']])
def test_how_to_type(xkbcli_how_to_type, args):
    xkbcli_how_to_type.run_command_success(args)


@pytest.mark.parametrize('rmlvos', rmlvos)
def test_how_to_type_rmlvo(xkbcli_how_to_type, rmlvos):
    args = rmlvos + ['0x1234']
    xkbcli_how_to_type.run_command_success(args)


@pytest.mark.parametrize('args', [['--verbose'],
                                  ['-v'],
                                  ['--verbose', '--load-exotic'],
                                  ['--load-exotic'],
                                  ['--ruleset=evdev'],
                                  ['--ruleset=base'],
                                  ])
def test_list_rmlvo(xkbcli_list, args):
    xkbcli_list.run_command_success(args)


def test_list_rmlvo_includes(xkbcli_list):
    args = ['/tmp/']
    xkbcli_list.run_command_success(args)


def test_list_rmlvo_includes_invalid(xkbcli_list):
    args = ['/proc/version']
    rc, stdout, stderr = xkbcli_list.run_command(args)
    assert rc == 1
    assert "Failed to append include path" in stderr


def test_list_rmlvo_includes_no_defaults(xkbcli_list):
    args = ['--skip-default-paths', '/tmp']
    rc, stdout, stderr = xkbcli_list.run_command(args)
    assert rc == 1
    assert "Failed to parse XKB description" in stderr


@pytest.mark.skipif(not os.path.exists('/dev/input/event0'), reason='event node required')
@pytest.mark.skipif(not os.access('/dev/input/event0', os.R_OK), reason='insufficient permissions')
@pytest.mark.parametrize('rmlvos', rmlvos)
def test_interactive_evdev_rmlvo(xkbcli_interactive_evdev, rmlvos):
    return
    xkbcli_interactive_evdev.run_command_success(rmlvos)


@pytest.mark.skipif(not os.path.exists('/dev/input/event0'),
                    reason='event node required')
@pytest.mark.skipif(not os.access('/dev/input/event0', os.R_OK),
                    reason='insufficient permissions')
@pytest.mark.parametrize('args', [['--report-state-changes'],
                                  ['--enable-compose'],
                                  ['--consumed-mode=xkb'],
                                  ['--consumed-mode=gtk'],
                                  ['--without-x11-offset'],
                                  ])
def test_interactive_evdev(xkbcli_interactive_evdev, args):
    # Note: --enable-compose fails if $prefix doesn't have the compose tables
    # installed
    xkbcli_interactive_evdev.run_command_success(args)


@pytest.mark.skipif(not os.getenv('DISPLAY'), reason='DISPLAY not set')
def test_interactive_x11(xkbcli_interactive_x11):
    # To be filled in if we handle something other than --help
    pass


@pytest.mark.skipif(not os.getenv('WAYLAND_DISPLAY'),
                    reason='WAYLAND_DISPLAY not set')
def test_interactive_wayland(xkbcli_interactive_wayland):
    # To be filled in if we handle something other than --help
    pass


if __name__ == '__main__':
    with tempfile.TemporaryDirectory() as tmpdir:
        # libxkbcommon has fallbacks when XDG_CONFIG_HOME isn't set so we need
        # to override it with a known (empty) directory. Otherwise our test
        # behavior depends on the system the test is run on.
        os.environ['XDG_CONFIG_HOME'] = tmpdir
        # This needs to be separated if we do specific extra path testing
        os.environ['XKB_CONFIG_EXTRA_PATH'] = tmpdir

        sys.exit(pytest.main(args=[__file__]))
