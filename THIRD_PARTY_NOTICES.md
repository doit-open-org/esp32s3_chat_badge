# Third-party notices and licensing boundaries

This repository combines code and assets from several sources. The root `LICENSE` does not override licenses found in subdirectories.

## xiaozhi-esp32

The firmware is derived from `78/xiaozhi-esp32`. The imported upstream project declares the MIT License. Existing copyright notices must be retained.

## PageManager / X-TRACK-derived code

`main/display/page_manager/` contains its own `LICENSE`, the GNU General Public License version 3. Its README attributes the design to FASTSHIFT's X-TRACK PageManager. Modifications and binary distributions that include this code must comply with the GPL-3.0 terms. In particular, distributing a linked firmware image may create source-disclosure and corresponding-source obligations for the combined work.

## VB6824 component

`components/vb6824/idf_component.yml` declares MIT and references `https://github.com/78/vb6824`. This repository also contains precompiled static libraries under `components/vb6824/libs/` for ESP-IDF 5.4 and 5.5. Maintainers should verify that the right to redistribute those exact binary files covers public source repositories and commercial firmware releases.

## Espressif and registry components

Components copied into `components/` or downloaded by the ESP-IDF Component Manager retain their own licenses and notices. Examples include ESP-IDF, ESP LVGL Port, ESP LV Decoder, ESP-SR, TinyUSB, LVGL, and codec/display drivers. Consult each component manifest and source package.

## Fonts, audio, images, and product assets

Generated C font arrays, audio prompts, product logos, UI artwork, and animations may have rights separate from the firmware source code. Their inclusion in this repository is not a representation that third-party trademarks or copyrighted creative assets are licensed for every form of redistribution. The product owner should retain an asset-origin and authorization record.

The unused legacy LittleFS material bundle from the internal source tree is deliberately excluded. Runtime mini-program/VPG material is stored on microSD and is supplied separately by the device owner or companion application.

## No legal advice

This notice identifies visible licensing boundaries for maintainers. It is not legal advice and does not replace review by the rights holder or counsel before public binary distribution.
