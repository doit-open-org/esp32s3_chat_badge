# Contributing

## Before opening a pull request

1. Use ESP-IDF v5.4.4 unless the change explicitly targets another supported version.
2. Run `python3 tools/check_repo.py` and `./tools/build.sh` from a clean build directory.
3. Test on MZ46-S3-DZ V1.0 hardware when changing pins, display, touch, audio, SD, USB, power, boot flow, or partitioning.
4. Keep the application name `xiaozhi` and board identifier `mz46-s3-dz` unless the compatibility impact is documented.
5. Do not commit `sdkconfig`, `dependencies.lock` with absolute paths, build output, firmware binaries, credentials, customer data, or manufacturing secrets.

## Code changes

- Prefer focused changes over broad refactors.
- Preserve ESP-IDF error checking and log actionable failures.
- Avoid file-system access while USB MSC owns the SD card.
- Document any pin, partition, protocol, endpoint, or persistent-storage change.
- Keep generated language headers out of Git; CMake regenerates them.

## Pull request information

Include the problem, implementation, test hardware, ESP-IDF version, relevant logs, and compatibility or migration impact. UI changes should include screenshots; hardware changes should include an updated pin table or schematic reference.

By contributing, you confirm that you have the right to submit the code and assets under their stated licenses.
