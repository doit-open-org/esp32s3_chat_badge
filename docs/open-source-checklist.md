# Open-source readiness checklist

- [ ] GitHub Actions completes an ESP-IDF v5.4 build from a clean clone.
- [ ] A maintainer reproduces the build locally without the original internal Git server.
- [ ] All product assets are approved for public redistribution.
- [ ] The legacy HTTP OTA endpoint is intentionally retained or replaced.
- [ ] No manufacturing keys, Wi-Fi credentials, access tokens, certificates, MAC lists, or customer data are present.
- [ ] MZ46-S3-DZ V1.0 hardware passes the regression list in `docs/releasing.md`.
- [ ] The merged image and source commit are linked by SHA-256 and release notes.
- [ ] GPL-3.0 and all third-party notices are included with binary distributions.
- [ ] The uploaded historical binary is labeled as historical and not claimed to be reproducible from a different source commit.
- [ ] Repository branch protection and required CI checks are enabled.
