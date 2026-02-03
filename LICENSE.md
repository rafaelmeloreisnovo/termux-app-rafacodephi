# License Information

## Fork Notice

**This is a fork of the original Termux project.**

- **Original Repository**: [termux/termux-app](https://github.com/termux/termux-app)
- **Original Copyright**: © The Termux developers and contributors
- **Fork Repository**: [instituto-Rafael/termux-app-rafacodephi](https://github.com/instituto-Rafael/termux-app-rafacodephi)
- **Fork Copyright**: © instituto-Rafael and RafaCodePhi contributors (for modifications)

This fork maintains all original copyright notices and complies fully with the GPLv3 license of the upstream project.

---

## Primary License

The `termux/termux-app` repository is released under [GPLv3 only](https://www.gnu.org/licenses/gpl-3.0.html) license.

### Exceptions

- [Terminal Emulator for Android](https://github.com/jackpal/Android-Terminal-Emulator) code is used which is released under [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0) license. Check [`terminal-view`](terminal-view) and [`terminal-emulator`](terminal-emulator) libraries.
- Check [`termux-shared/LICENSE.md`](termux-shared/LICENSE.md) for `termux-shared` library related exceptions.

---

## Detailed Attribution

### Component Licenses

This project incorporates code from multiple sources with different licenses:

#### 1. GPLv3 Components
- **Main Application Code**: The primary Termux application
- **Copyright**: © Termux developers and contributors
- **License**: [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html)
- **License Requirements**: 
  - Source code must be made available
  - Modifications must be released under GPLv3
  - Copyright and license notices must be preserved
  - Changes must be documented

#### 2. Apache 2.0 Components
- **Terminal Emulator for Android**: Terminal emulation libraries
- **Copyright**: © Jack Palevich and contributors
- **Source**: [jackpal/Android-Terminal-Emulator](https://github.com/jackpal/Android-Terminal-Emulator)
- **License**: [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0)
- **License Requirements**:
  - Copyright notice and license must be included
  - Notice of modifications must be provided
  - Patent rights are granted

#### 3. MIT Components
- **termux-shared library** (with exceptions)
- **Copyright**: © Termux developers and contributors
- **License**: [MIT License](https://opensource.org/licenses/MIT)
- **Exceptions**: See [termux-shared/LICENSE.md](termux-shared/LICENSE.md) for GPLv3 exceptions

#### 4. GPLv2 with Classpath Exception
- **Filesystem utilities** from Android AOSP
- **Copyright**: © The Android Open Source Project
- **Source**: [Android libcore/ojluni](https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/)
- **License**: [GPLv2 with Classpath Exception](https://openjdk.java.net/legal/gplv2+ce.html)

---

## Copyright Notices

### Original Project Copyright
```
Copyright (c) 2015-2024 Termux developers and contributors
```

### Fork Copyright
```
Copyright (c) 2024-present instituto-Rafael and RafaCodePhi contributors
```

All modifications and additions made in this fork are also released under the GPLv3 license to maintain compatibility with the upstream project.

---

## Legal Compliance Statement

This project complies with:

1. **GNU General Public License v3.0** - The primary license governing the Termux application
2. **Apache License 2.0** - For components derived from the Terminal Emulator for Android
3. **MIT License** - For applicable termux-shared library components
4. **GPLv2 with Classpath Exception** - For filesystem utilities from AOSP
5. **International Copyright Law** - All copyright notices are preserved
6. **Intellectual Property Regulations** - Proper attribution is provided for all sources

### Copyleft Provision
As required by GPLv3, this entire work and any derivative works must be distributed under GPLv3. The source code is made available at [https://github.com/instituto-Rafael/termux-app-rafacodephi](https://github.com/instituto-Rafael/termux-app-rafacodephi).

### Patent Grant
Components under Apache 2.0 license include an express patent grant from contributors.

### Attribution Requirements
All original copyright notices, license texts, and attribution information must be preserved in any redistribution or derivative work. Even minor contributions (including single character or punctuation changes) are recognized as copyrightable modifications under copyright law.

---

## License Texts

### Full License Texts Available At:

- **GPLv3**: [https://www.gnu.org/licenses/gpl-3.0.txt](https://www.gnu.org/licenses/gpl-3.0.txt)
- **Apache 2.0**: [https://www.apache.org/licenses/LICENSE-2.0.txt](https://www.apache.org/licenses/LICENSE-2.0.txt)
- **MIT**: [https://opensource.org/licenses/MIT](https://opensource.org/licenses/MIT)
- **GPLv2+CE**: [https://openjdk.java.net/legal/gplv2+ce.html](https://openjdk.java.net/legal/gplv2+ce.html)

### In Repository:
- This file (LICENSE.md) for main application
- [termux-shared/LICENSE.md](termux-shared/LICENSE.md) for shared library specifics
- Individual source files may contain additional copyright and license headers

---

## Warranty Disclaimer

THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This disclaimer applies to the original Termux project, this fork, and all incorporated components under their respective licenses.

---

## RAFAELIA Framework Attribution

### Acknowledgment of RAFAELIA Methodology

This fork incorporates the **RAFAELIA Framework** - a modular, matrix-based computational methodology developed by instituto-Rafael. RAFAELIA represents a systematic approach to low-level programming that emphasizes:

- **Matrix-based computation**: Using deterministic linear algebra for problem solving
- **Minimal dependencies**: Self-contained implementations without external libraries
- **Architectural optimization**: Hardware-specific SIMD optimizations (NEON, AVX, SSE)
- **Ethical computation**: Following principles of Φ_ethica (ethical filter) in all operations
- **Modular design**: Clean separation between Java interfaces and C/ASM implementations

#### RAFAELIA Core Principles

The RAFAELIA methodology is guided by the following principles:

1. **Humildade_Ω (Humility)**: Acknowledge what is known and unknown; proceed iteratively
2. **Coerência (Coherence)**: Maintain consistency across all layers of the system
3. **Φ_ethica (Ethical Filter)**: Minimize entropy, maximize coherence in all decisions
4. **Retroalimentação (Feedback)**: Continuous feedback loops for improvement (ψχρΔΣΩ cycle)
5. **Determinismo (Determinism)**: Matrix operations with predictable, verifiable outcomes

#### RAFAELIA Authorship Note

The following components are original contributions under the RAFAELIA framework:

- **Low-level matrix operations** (`app/src/main/cpp/lowlevel/`): Pure C/ASM implementations without external dependencies
- **Bare-metal utilities** (`rafaelia/`): SIMD-optimized vector and matrix operations
- **Deterministic flip operations**: Matrix solving through horizontal, vertical, and diagonal transformations
- **Architecture-optimized kernels**: Platform-specific NEON/AVX/SSE implementations

**Copyright**: © 2024-present instituto-Rafael  
**Methodology**: RAFAELIA Framework (RAFCODE-Φ)  
**License**: GPLv3 (maintaining compatibility with upstream Termux)  
**Attribution Required**: When using RAFAELIA components, please acknowledge:
  - "Powered by RAFAELIA Framework - instituto-Rafael"
  - Reference to this repository and methodology

#### RAFAELIA Mathematical Foundation

The RAFAELIA framework is built on rigorous mathematical foundations:

```
R_Ω = Σ_n (ψ_n·χ_n·ρ_n·Δ_n·Σ_n·Ω_n)^{Φλ}
```

Where:
- **ψ** (psi): Perception and input processing
- **χ** (chi): Feedback and retroalignment
- **ρ** (rho): Expansion and transformation
- **Δ** (Delta): Validation and verification
- **Σ** (Sigma): Execution and synthesis
- **Ω** (Omega): Ethical alignment and completion
- **Φλ** (Phi-lambda): Coherence scaling factor

This cycle ensures that all computational operations maintain ethical coherence (Φ_ethica) and deterministic outcomes through matrix-based transformations.

#### Technical Implementation Notes

1. **Matrix Structures**: All matrices use minimal structure `mx_t` with direct memory access
2. **Variable Naming**: Intentionally minimal (m, r, c) to reduce abstraction overhead
3. **Flip Operations**: Deterministic matrix solving via transformations:
   - `mx_flip_h()`: Horizontal flip for row-wise operations
   - `mx_flip_v()`: Vertical flip for column-wise operations  
   - `mx_flip_d()`: Diagonal flip (transpose) for linear system solving
4. **No Legacy Dependencies**: All functions have new names (e.g., `fm_sqrt`, `vop_dot`, `bmem_cpy`)
5. **Hardware Optimization**: Automatic detection and use of NEON/AVX/SSE SIMD instructions

#### Gratitude and Acknowledgments

The RAFAELIA framework acknowledges and thanks:

- **Original Termux Project**: For providing the foundation and terminal infrastructure
- **Android Open Source Project**: For filesystem utilities and platform support
- **Open Source Community**: For mathematical algorithms and optimization techniques that inspired RAFAELIA's implementation
- **Contributors**: All individuals who have contributed to the evolution of this methodology

**Special Note**: While RAFAELIA implements its own low-level primitives, it stands on the shoulders of decades of computer science research. We acknowledge that mathematical techniques (Newton-Raphson, SIMD vectorization, linear algebra methods) are part of the collective knowledge of humanity and are used here in the spirit of advancing accessible, ethical computation.

---

## Questions and Attribution Requests

For questions about licensing, attribution, or to report missing attributions:
- **Original Termux**: See [https://termux.com](https://termux.com)
- **RAFAELIA Framework**: Open an issue at [https://github.com/instituto-Rafael/termux-app-rafacodephi/issues](https://github.com/instituto-Rafael/termux-app-rafacodephi/issues)
- **This Fork**: Open an issue at [https://github.com/instituto-Rafael/termux-app-rafacodephi/issues](https://github.com/instituto-Rafael/termux-app-rafacodephi/issues)
