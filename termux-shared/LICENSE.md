# License Information - termux-shared Library

## Fork Notice

**This is part of a fork of the original Termux project.**

- **Original Repository**: [termux/termux-app](https://github.com/termux/termux-app)
- **Original Copyright**: © The Termux developers and contributors
- **Fork Repository**: [instituto-Rafael/termux-app-rafacodephi](https://github.com/instituto-Rafael/termux-app-rafacodephi)

This library maintains all original copyright notices and complies with the original licensing terms.

---

## Primary License

The `termux-shared` library is released under [MIT](https://opensource.org/licenses/MIT) license.

### Exceptions

#### [GPLv3 only](https://www.gnu.org/licenses/gpl-3.0.html)

- [`src/main/java/com/termux/shared/termux/*`](src/main/java/com/termux/shared/termux).

The `GPLv3 only` license applies to all files unless specifically specified by a file/directory, like the [`src/main/java/com/termux/shared/termux/TermuxConstants.java`](src/main/java/com/termux/shared/termux/TermuxConstants.java) and [`src/main/java/com/termux/shared/termux/settings/properties/TermuxPropertyConstants.java`](src/main/java/com/termux/shared/termux/settings/properties/TermuxPropertyConstants.java) files are released under the `MIT` license.
##


#### [GPLv2 only with "Classpath" exception](https://openjdk.java.net/legal/gplv2+ce.html)

- [`src/main/java/com/termux/shared/file/filesystem/*`](src/main/java/com/termux/shared/file/filesystem) files that use code from [libcore/ojluni](https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/).
##


#### [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0)

- [`src/main/java/com/termux/shared/shell/StreamGobbler.java`](src/main/java/com/termux/shared/shell/StreamGobbler.java) uses code from [libsuperuser ](https://github.com/Chainfire/libsuperuser).
##

---

## Detailed Component Attribution

### 1. MIT Licensed Components
- **Main library code** (except where exceptions apply)
- **Copyright**: © Termux developers and contributors
- **License**: [MIT License](https://opensource.org/licenses/MIT)
- **Files**: All files not covered by exceptions below

### 2. GPLv3 Licensed Components
- **Termux-specific utilities and constants**
- **Copyright**: © Termux developers and contributors
- **License**: [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html)
- **Location**: `src/main/java/com/termux/shared/termux/*`
- **MIT Exceptions within GPLv3 directory**:
  - `TermuxConstants.java`
  - `settings/properties/TermuxPropertyConstants.java`

### 3. GPLv2 with Classpath Exception Components
- **Filesystem utilities derived from Android AOSP**
- **Copyright**: © The Android Open Source Project
- **License**: [GPLv2 with Classpath Exception](https://openjdk.java.net/legal/gplv2+ce.html)
- **Source**: [Android libcore/ojluni](https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/)
- **Location**: `src/main/java/com/termux/shared/file/filesystem/*`

### 4. Apache 2.0 Licensed Components
- **StreamGobbler utility**
- **Copyright**: © Chainfire
- **License**: [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0)
- **Source**: [libsuperuser](https://github.com/Chainfire/libsuperuser)
- **Location**: `src/main/java/com/termux/shared/shell/StreamGobbler.java`

---

## Copyright Notices

### Original Copyright
```
Copyright (c) 2015-2024 Termux developers and contributors
```

### Third-Party Copyrights
- **Android Open Source Project**: For filesystem utilities
- **Chainfire**: For StreamGobbler implementation
- **Jack Palevich and contributors**: For terminal emulation components

### Fork Copyright
```
Copyright (c) 2024-present instituto-Rafael and RafaCodePhi contributors
```
(Applies only to modifications made in this fork)

---

## Legal Compliance

This library component complies with:

1. **MIT License** - For the primary library code
2. **GPLv3** - For Termux-specific utilities
3. **GPLv2 with Classpath Exception** - For AOSP-derived filesystem utilities
4. **Apache 2.0** - For libsuperuser-derived code
5. **Copyright Law** - All attributions and notices are preserved

### Important Notes

- Each component must be used according to its specific license
- GPLv3 components may affect the overall license of derivative works
- The Classpath Exception allows linking with independent modules
- Apache 2.0 includes patent grant provisions
- All modifications are documented and attributed

---

## Full License Texts

- **MIT**: [https://opensource.org/licenses/MIT](https://opensource.org/licenses/MIT)
- **GPLv3**: [https://www.gnu.org/licenses/gpl-3.0.txt](https://www.gnu.org/licenses/gpl-3.0.txt)
- **GPLv2+CE**: [https://openjdk.java.net/legal/gplv2+ce.html](https://openjdk.java.net/legal/gplv2+ce.html)
- **Apache 2.0**: [https://www.apache.org/licenses/LICENSE-2.0.txt](https://www.apache.org/licenses/LICENSE-2.0.txt)
