# Security policy

## Reporting a vulnerability

Do not publish exploitable details, device credentials, private OTA URLs, signing keys, or customer data in a public Issue. Use the repository's **Security → Report a vulnerability** workflow to create a private GitHub Security Advisory.

Include the affected commit or release, hardware revision, reproduction steps, impact, and any proposed mitigation. Maintainers should acknowledge the report privately, reproduce it, prepare a coordinated fix, and publish an advisory when users can upgrade safely.

## Supported versions

Until the first verified public release is tagged, only the current `main` branch receives security fixes. After releases begin, the project should document a supported-version window here.

## Sensitive configuration

Production forks must provide their own authenticated OTA infrastructure, TLS policy, device credentials, and signing process. The sample/default endpoints and empty access-token placeholder are not a production authentication design.
