# FujiNet on the MiSTer: binaries snapshot (2026-09-27)

A snapshot of the FujiNet binaries and files from the MiSTer
(DE10-Nano) at `10.3.89.233`, taken **2026-09-27**, plus the FujiNet build of
the MiSTer `Main` binary that was on the box.

This is the FujiNet-PC firmware (`FujiNet v1.6-6ae6884* 2026-09-24 (COCO)`,
per `fujinet.log`) cross-built for the MiSTer ARM HPS, serving DriveWire to the
CoCo3 core over `/dev/ttyS1` at 19200 baud, with its web UI on port 8000.
See `media_fat/fujinet/README.md` for the package's own setup notes.

## Layout

| folder | came from |
|---|---|
| `mister-main/` | `/media/fat/` on the MiSTer (the Main binary) |
| `media_fat/fujinet/` | `/media/fat/fujinet/` (the whole FujiNet install, 19 MB on disk) |
| `media_fat/Scripts/` | `/media/fat/Scripts/` (the OSD start/stop script) |

To use the Main build, copy `mister-main/MiSTer.fujinet-20260925` to
`/media/fat/MiSTer` on a MiSTer (keep a backup of the original) and reboot.

## Files

Sizes in bytes; md5 verified against `md5sum` run on the MiSTer itself.

| repo path | original path on the MiSTer | size | md5 | description |
|---|---|---:|---|---|
| `mister-main/MiSTer.fujinet-20260925` | `/media/fat/MiSTer.fujinet-20260925` | 1067756 | `72152e8710ff6d76951fdcdd07428469` | ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV) |
| `media_fat/Scripts/fujinet.sh` | `/media/fat/Scripts/fujinet.sh` | 551 | `06cf2b8a5558a927c2bf29bdbc6590fb` | Bourne-Again shell script, ASCII text executable |
| `media_fat/fujinet/README.md` | `/media/fat/fujinet/README.md` | 2396 | `542c970e5476b92b28cd525a3edad070` | ASCII text |
| `media_fat/fujinet/SD/--- FujiNet SD Card ---` | `/media/fat/fujinet/SD/--- FujiNet SD Card ---` | 0 | `d41d8cd98f00b204e9800998ecf8427e` | empty |
| `media_fat/fujinet/SD/autorun.dsk` | `/media/fat/fujinet/SD/autorun.dsk` | 161280 | `81c78ed40dabe219248fed5a7cf0c082` | data |
| `media_fat/fujinet/SD/autorund.vdk` | `/media/fat/fujinet/SD/autorund.vdk` | 368652 | `d978d357c5013ae84492f946b85e279f` | data |
| `media_fat/fujinet/SD/mount-and-boot.dsk` | `/media/fat/fujinet/SD/mount-and-boot.dsk` | 161280 | `21f8887f9189c87c4695699d28c4ee63` | data |
| `media_fat/fujinet/data/AUTOLOAD.DWL` | `/media/fat/fujinet/data/AUTOLOAD.DWL` | 1767 | `2c15969b54630e79aeebdbca2c2c7d11` | data |
| `media_fat/fujinet/data/CONFIG.DWL` | `/media/fat/fujinet/data/CONFIG.DWL` | 19239 | `640ae8e84b3e6a93bad620ed2aa4c756` | data |
| `media_fat/fujinet/data/DGNLOBBY.DWL` | `/media/fat/fujinet/data/DGNLOBBY.DWL` | 8491 | `e1df325c735fdabd8e15e48075384358` | data |
| `media_fat/fujinet/data/LOGO.DWL` | `/media/fat/fujinet/data/LOGO.DWL` | 525 | `a18210a556c11be667cfcd79ff201dda` | data |
| `media_fat/fujinet/data/STAGE2.DWL` | `/media/fat/fujinet/data/STAGE2.DWL` | 1056 | `2760e08e9d616565e5dd51ba36cbf91e` | data |
| `media_fat/fujinet/data/atarifont.css` | `/media/fat/fujinet/data/atarifont.css` | 8569 | `28a6892d2b3292d28f9c1b1e16374d2d` | ASCII text, with very long lines (8398) |
| `media_fat/fujinet/data/autorun.dsk` | `/media/fat/fujinet/data/autorun.dsk` | 161280 | `81c78ed40dabe219248fed5a7cf0c082` | data |
| `media_fat/fujinet/data/autorund.vdk` | `/media/fat/fujinet/data/autorund.vdk` | 368652 | `d978d357c5013ae84492f946b85e279f` | data |
| `media_fat/fujinet/data/ca.pem` | `/media/fat/fujinet/data/ca.pem` | 29168 | `b8930dfcfc64ecdf47a2d986464b2692` | PEM certificate |
| `media_fat/fujinet/data/f/a1025/F1` | `/media/fat/fujinet/data/f/a1025/F1` | 7163 | `3d1bc8e155725f871867ade5dc3d58cb` | data |
| `media_fat/fujinet/data/f/a1025/F2` | `/media/fat/fujinet/data/f/a1025/F2` | 6099 | `8ebda495142bb98828a7ea95b4ac2bba` | data |
| `media_fat/fujinet/data/f/a1025/F3` | `/media/fat/fujinet/data/f/a1025/F3` | 4755 | `c6f2a244d07d69f0603dcf3b815e350f` | data |
| `media_fat/fujinet/data/f/a1025/LUT` | `/media/fat/fujinet/data/f/a1025/LUT` | 92 | `9995f2a5f25c874c0c4e8e0157f99c73` | ASCII text |
| `media_fat/fujinet/data/f/a1027/F1` | `/media/fat/fujinet/data/f/a1027/F1` | 22732 | `70b68952258b1d4a5d8121a943eb82d3` | data |
| `media_fat/fujinet/data/f/a1027/LUT` | `/media/fat/fujinet/data/f/a1027/LUT` | 34 | `dcf0d900f2c2ad2fdc85912f9315fa3c` | ASCII text |
| `media_fat/fujinet/data/f/a1029/F1` | `/media/fat/fujinet/data/f/a1029/F1` | 10606 | `2bfc43c1ecbfbc3850a7dcbfd2e7660c` | data |
| `media_fat/fujinet/data/f/a1029/F2` | `/media/fat/fujinet/data/f/a1029/F2` | 11051 | `918a77ed2888948f3d9e89ed35a8f9bc` | data |
| `media_fat/fujinet/data/f/a1029/F3` | `/media/fat/fujinet/data/f/a1029/F3` | 16530 | `cba6055e7ceb01a81abbe66fa368638b` | data |
| `media_fat/fujinet/data/f/a1029/F4` | `/media/fat/fujinet/data/f/a1029/F4` | 10881 | `4253e7f38dbb1f7c0e86fac78639e3fb` | data |
| `media_fat/fujinet/data/f/a1029/F5` | `/media/fat/fujinet/data/f/a1029/F5` | 2052 | `ab26989c3612f5f9aa5b9138e96efac8` | data |
| `media_fat/fujinet/data/f/a1029/LUT` | `/media/fat/fujinet/data/f/a1029/LUT` | 158 | `e7f68b922f9875f8fb64827c2c5e4503` | ASCII text |
| `media_fat/fujinet/data/f/a820/F1` | `/media/fat/fujinet/data/f/a820/F1` | 5706 | `7b6f5387b6a76803b749bf21fa76dce5` | data |
| `media_fat/fujinet/data/f/a820/F2` | `/media/fat/fujinet/data/f/a820/F2` | 5295 | `82c9beb9911abff1102cc0b98fcf50c6` | data |
| `media_fat/fujinet/data/f/a820/LUT` | `/media/fat/fujinet/data/f/a820/LUT` | 62 | `fb5821e04004d034fe0f730548bd41fb` | ASCII text |
| `media_fat/fujinet/data/f/a822/F1` | `/media/fat/fujinet/data/f/a822/F1` | 2990 | `45f008486452b5449336ffb9c8c13ec5` | data |
| `media_fat/fujinet/data/f/a822/LUT` | `/media/fat/fujinet/data/f/a822/LUT` | 32 | `f8c24a1da23767af6b905b3f7fd6f079` | ASCII text |
| `media_fat/fujinet/data/f/a825/F1` | `/media/fat/fujinet/data/f/a825/F1` | 5667 | `f8e12090d92ea9bc46ba95eab24334c0` | data |
| `media_fat/fujinet/data/f/a825/F10` | `/media/fat/fujinet/data/f/a825/F10` | 13766 | `f95bb5a3f9d395e62b0e622b003c7cd0` | data |
| `media_fat/fujinet/data/f/a825/F11` | `/media/fat/fujinet/data/f/a825/F11` | 10183 | `d14db4e59653964417690173e57d41b8` | data |
| `media_fat/fujinet/data/f/a825/F12` | `/media/fat/fujinet/data/f/a825/F12` | 10624 | `7c8fd0dde616bda90f88de784a8e985f` | data |
| `media_fat/fujinet/data/f/a825/F2` | `/media/fat/fujinet/data/f/a825/F2` | 5802 | `1ef7ac2a3615287089a425c3e93adf3e` | data |
| `media_fat/fujinet/data/f/a825/F3` | `/media/fat/fujinet/data/f/a825/F3` | 8182 | `1ffcb18b3b7f787081611a0aa0578a62` | data |
| `media_fat/fujinet/data/f/a825/F4` | `/media/fat/fujinet/data/f/a825/F4` | 6677 | `15b97e8f00e7f1494a3d03534e94df88` | data |
| `media_fat/fujinet/data/f/a825/F5` | `/media/fat/fujinet/data/f/a825/F5` | 6691 | `190726bef772e100da97b2fccb759943` | data |
| `media_fat/fujinet/data/f/a825/F6` | `/media/fat/fujinet/data/f/a825/F6` | 6474 | `c913672f197e7b3be9013744dee31777` | data |
| `media_fat/fujinet/data/f/a825/F7` | `/media/fat/fujinet/data/f/a825/F7` | 8119 | `5a64802e9b6a830f97b6b28a401c58cf` | data |
| `media_fat/fujinet/data/f/a825/F8` | `/media/fat/fujinet/data/f/a825/F8` | 6307 | `f0622786e60db25768a8f9ce6487f9d1` | data |
| `media_fat/fujinet/data/f/a825/F9` | `/media/fat/fujinet/data/f/a825/F9` | 23867 | `927a4401acd48032a711c0608f448ded` | data |
| `media_fat/fujinet/data/f/a825/LUT` | `/media/fat/fujinet/data/f/a825/LUT` | 369 | `b5b94f11da1169680fe6dcd3b52dc507` | ASCII text |
| `media_fat/fujinet/data/f/epson/F1` | `/media/fat/fujinet/data/f/epson/F1` | 8769 | `ab374d44990acea317ba08eb63b16b23` | data |
| `media_fat/fujinet/data/f/epson/F10` | `/media/fat/fujinet/data/f/epson/F10` | 16881 | `01a03e26efe2b01df51e651aab31872b` | data |
| `media_fat/fujinet/data/f/epson/F11` | `/media/fat/fujinet/data/f/epson/F11` | 17339 | `4e215107619dc941630deb4d11214381` | data |
| `media_fat/fujinet/data/f/epson/F12` | `/media/fat/fujinet/data/f/epson/F12` | 18752 | `8794eaaf4d8daabb5207121c0fef58c5` | data |
| `media_fat/fujinet/data/f/epson/F13` | `/media/fat/fujinet/data/f/epson/F13` | 11363 | `108511856a958c9d4887686b1b151c13` | data |
| `media_fat/fujinet/data/f/epson/F14` | `/media/fat/fujinet/data/f/epson/F14` | 20341 | `83e8db8a98272c563197645dd621d9a2` | data |
| `media_fat/fujinet/data/f/epson/F15` | `/media/fat/fujinet/data/f/epson/F15` | 2469 | `cf8b88b4c926ec45953c4b65a4a8d88a` | data |
| `media_fat/fujinet/data/f/epson/F2` | `/media/fat/fujinet/data/f/epson/F2` | 17087 | `e120932f7b3f61980509d12723ba2446` | data |
| `media_fat/fujinet/data/f/epson/F3` | `/media/fat/fujinet/data/f/epson/F3` | 8845 | `7ca2ba81d5c8b7f287268be5634ed0b5` | data |
| `media_fat/fujinet/data/f/epson/F4` | `/media/fat/fujinet/data/f/epson/F4` | 16236 | `f63076dfe2577849722a403a5e44aba2` | data |
| `media_fat/fujinet/data/f/epson/F5` | `/media/fat/fujinet/data/f/epson/F5` | 9393 | `a2b0d66749ea0053cf15713acc608ed1` | data |
| `media_fat/fujinet/data/f/epson/F6` | `/media/fat/fujinet/data/f/epson/F6` | 18201 | `a740a650204cc3017afcdb25bc5efe93` | data |
| `media_fat/fujinet/data/f/epson/F7` | `/media/fat/fujinet/data/f/epson/F7` | 9364 | `53b3d9fef9fea4c3e2ce5913dec2c01d` | data |
| `media_fat/fujinet/data/f/epson/F8` | `/media/fat/fujinet/data/f/epson/F8` | 17386 | `631150e91e206b56d38748e1aea5347d` | data |
| `media_fat/fujinet/data/f/epson/F9` | `/media/fat/fujinet/data/f/epson/F9` | 15849 | `ece9140eb7825090d56a674fc5e07a25` | data |
| `media_fat/fujinet/data/f/epson/LUT` | `/media/fat/fujinet/data/f/epson/LUT` | 473 | `b326ceeabe93da99ce24f8d53d4ac4e8` | ASCII text |
| `media_fat/fujinet/data/f/mps803/F1` | `/media/fat/fujinet/data/f/mps803/F1` | 5457 | `bad8aa67c6bfd44ce6ff6f6053d4649c` | data |
| `media_fat/fujinet/data/f/mps803/F2` | `/media/fat/fujinet/data/f/mps803/F2` | 6887 | `a96a3c7011dd72e8fc19bfcd1ac21a13` | data |
| `media_fat/fujinet/data/f/mps803/F3` | `/media/fat/fujinet/data/f/mps803/F3` | 5432 | `e6d247af6e6cd00c45555593c2fc4d93` | data |
| `media_fat/fujinet/data/f/mps803/F4` | `/media/fat/fujinet/data/f/mps803/F4` | 7116 | `bacf9e3a2598b9771af8ab2abad64ed8` | data |
| `media_fat/fujinet/data/f/mps803/F5` | `/media/fat/fujinet/data/f/mps803/F5` | 2162 | `c53449d84624b71219118661b093694e` | data |
| `media_fat/fujinet/data/f/mps803/LUT` | `/media/fat/fujinet/data/f/mps803/LUT` | 152 | `c99db6275aebfc11817bb26c4658d786` | ASCII text |
| `media_fat/fujinet/data/f/oki10/F1` | `/media/fat/fujinet/data/f/oki10/F1` | 8892 | `77229080cfae88af29508ba916591a5a` | data |
| `media_fat/fujinet/data/f/oki10/F2` | `/media/fat/fujinet/data/f/oki10/F2` | 2144 | `64c8ac2f31c9b9d8caf761e2107e62ec` | data |
| `media_fat/fujinet/data/f/oki10/LUT` | `/media/fat/fujinet/data/f/oki10/LUT` | 62 | `145c9bf16a867ef149b850822849e0dd` | ASCII text |
| `media_fat/fujinet/data/f/xdm121/F1` | `/media/fat/fujinet/data/f/xdm121/F1` | 38688 | `7930b0395ea5da3268a1cd679804ce22` | data |
| `media_fat/fujinet/data/f/xdm121/F2` | `/media/fat/fujinet/data/f/xdm121/F2` | 43302 | `e9f57ff978159284d0a57eaa6a107286` | data |
| `media_fat/fujinet/data/f/xdm121/LUT` | `/media/fat/fujinet/data/f/xdm121/LUT` | 66 | `50025f26cd063e7cad05ee2cdd68ab42` | ASCII text |
| `media_fat/fujinet/data/fnconfig.ini` | `/media/fat/fujinet/data/fnconfig.ini` | 176 | `8c968bfef4087816ff1ebff9db31e83d` | ASCII text |
| `media_fat/fujinet/data/mount-and-boot.dsk` | `/media/fat/fujinet/data/mount-and-boot.dsk` | 161280 | `21f8887f9189c87c4695699d28c4ee63` | data |
| `media_fat/fujinet/data/www/FuturaBT-Light.woff2` | `/media/fat/fujinet/data/www/FuturaBT-Light.woff2` | 18860 | `59588ffb5413b2b3fcda97fe5e8bcf71` | Web Open Font Format (Version 2), TrueType, length 18860 |
| `media_fat/fujinet/data/www/Inconsolata-Light.woff2` | `/media/fat/fujinet/data/www/Inconsolata-Light.woff2` | 41624 | `2cac6b1280c4f067a379a369f10a53ae` | Web Open Font Format (Version 2), TrueType, length 41624 |
| `media_fat/fujinet/data/www/css/core.css` | `/media/fat/fujinet/data/www/css/core.css` | 17675 | `5b5451974c8b635f25ac58b357c3bc95` | ASCII text, with very long lines (4353) |
| `media_fat/fujinet/data/www/error_page.html` | `/media/fat/fujinet/data/www/error_page.html` | 143 | `577687a5417f35ef4086f812d8aa8921` | HTML document, ASCII text |
| `media_fat/fujinet/data/www/favicon.ico` | `/media/fat/fujinet/data/www/favicon.ico` | 12789 | `efeaf879fb1d89831198f30451ed1b66` | PNG image data, 512 x 501, 8-bit/color RGBA |
| `media_fat/fujinet/data/www/footer.html` | `/media/fat/fujinet/data/www/footer.html` | 15 | `4d214dd44eaca17a5b8c100a0cee7ebc` | ASCII text |
| `media_fat/fujinet/data/www/header.html` | `/media/fat/fujinet/data/www/header.html` | 16753 | `6fbcc6c37d6e25c5a01b5d12eeb85187` | HTML document, ASCII text, with very long lines (4365) |
| `media_fat/fujinet/data/www/index.html` | `/media/fat/fujinet/data/www/index.html` | 83649 | `f975d0a9b6ffa9306c5b2e0191d0786a` | HTML document, ASCII text, with very long lines (3126) |
| `media_fat/fujinet/data/www/js/select.js` | `/media/fat/fujinet/data/www/js/select.js` | 2832 | `375fc7438b64ff981a4407881597cf70` | ASCII text |
| `media_fat/fujinet/data/www/js/settings.js` | `/media/fat/fujinet/data/www/js/settings.js` | 5247 | `92f8678b3bf93a80ec2cfa662518debe` | ASCII text |
| `media_fat/fujinet/data/www/js/storage.js` | `/media/fat/fujinet/data/www/js/storage.js` | 1593 | `f66d63563d5b867839e02b238ca3b908` | ASCII text |
| `media_fat/fujinet/data/www/js/utils.js` | `/media/fat/fujinet/data/www/js/utils.js` | 1056 | `11bc8c2910a847621ee969d581598205` | JavaScript source, ASCII text |
| `media_fat/fujinet/data/www/logo.svg` | `/media/fat/fujinet/data/www/logo.svg` | 4986 | `ef83a83e193fae3090480208b57889e6` | SVG Scalable Vector Graphics image |
| `media_fat/fujinet/data/www/redirect_to_index.html` | `/media/fat/fujinet/data/www/redirect_to_index.html` | 188 | `96497e799579d306c2c7f88079201f67` | HTML document, ASCII text |
| `media_fat/fujinet/data/www/restart.html` | `/media/fat/fujinet/data/www/restart.html` | 14091 | `be6f293b016b22a9db00512464db5266` | HTML document, ASCII text, with very long lines (4352) |
| `media_fat/fujinet/fnconfig.ini` | `/media/fat/fujinet/fnconfig.ini` | 1450 | `8b3d5734fc9c625649ab4cc01dfa733e` | Generic INItialization configuration [WiFi] |
| `media_fat/fujinet/fujinet` | `/media/fat/fujinet/fujinet` | 3354132 | `be37092930bb379503a7946f0a7dfff9` | ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV) |
| `media_fat/fujinet/fujinet.log` | `/media/fat/fujinet/fujinet.log` | 16899 | `f86a6c9591b9bcbd135014cb4d4a098d` | ASCII text, with CRLF, LF line terminators |
| `media_fat/fujinet/roms/HDBDW3CC3.CCC` | `/media/fat/fujinet/roms/HDBDW3CC3.CCC` | 8192 | `36dc27a3f7d590ec5fd26e0b5f583034` | data |
| `media_fat/fujinet/roms/HDBDW3L3_19200.CCC` | `/media/fat/fujinet/roms/HDBDW3L3_19200.CCC` | 8192 | `0fed39e94c8daa47a4e35513a0220ec7` | data |
| `media_fat/fujinet/roms/HDBDW3S3.CCC` | `/media/fat/fujinet/roms/HDBDW3S3.CCC` | 8192 | `8330caf39d43b0b89f799be931559e8d` | data |
| `media_fat/fujinet/run-fujinet` | `/media/fat/fujinet/run-fujinet` | 284 | `bba83b8fa5b0e5488f4bdc592fa5eb42` | POSIX shell script, ASCII text executable |
| `media_fat/fujinet/start_fujinet.sh` | `/media/fat/fujinet/start_fujinet.sh` | 549 | `1306a35482c7d5a12790ae77ea85475c` | POSIX shell script, ASCII text executable |

The empty directory `/media/fat/fujinet/SD/FujiNet/` exists on the MiSTer too;
git does not track empty directories, so it is not in this repo.

## Left out

- **Other `MiSTer.*` Main binaries** on the box (`MiSTer`, `MiSTer.bak_pre_fujinet`
  = the Main from before FujiNet was added, printer / write-buffer / AppleIII
  builds, etc.): not FujiNet builds.
- **Unrelated name matches** from `find -iname '*fuji*'`: Fujifilm shadow masks,
  Fujitsu FM-7 disks and ROMs, and game ROMs whose titles contain "Fuji"
  (`Fujiama Run.zip`, etc.).
- **Secrets**: none found. `fnconfig.ini` has empty Wi-Fi SSID/passphrase and
  empty Google Drive / S3 / OneDrive token fields, and is included as-is.
  `data/ca.pem` is the public root-CA bundle (certificates only, no private keys).
- Nothing exceeded GitHub's 100 MB file limit (largest file is the 3.3 MB
  `fujinet` executable).

No FujiNet process was running on the MiSTer when the snapshot was taken.

## Source

Built from <https://github.com/alanswx/FujiNet_MiSTer> (wraps
<https://github.com/FujiNetWIFI/fujinet-firmware>). The `fujinet` executable
here is byte-identical to `fujinet_coco_demo/fujinet` on the build host.
