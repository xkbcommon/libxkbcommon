Merge modes and include mechanism were completely refactored to fix inconsistencies.
Included files are now always processed in isolation and do not propagate the local
merge modes. This makes the reasoning about included files much easier and consistent.
