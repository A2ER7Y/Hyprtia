# Third-party notices

## Brain_Shell

The StratOS session transition integration and adapted hyprlock layout are based
on Brain_Shell at commit `f90fc9c6bdfb25568c731ea1158d3f8e4b7a6e20`.

MIT License

Copyright (c) 2026 Venkat Saahit Kamu (Brainitech)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Design and compatibility references

The native StratOS bar layout is visually inspired by ActivSpot at commit
`19566260ff509a2ee0462aa2d8ee948e7b028c4d`:
<https://github.com/Devvvmn/ActivSpot> (GPL-3.0).

The `hyprtia-android-connect` compatibility launcher targets AndroidConnect at
commit `6f8172b6135b97cb5a878311ae3723fe4e5aa324`:
<https://github.com/demencia89/noctalia-shell-androidconnect-plugin> (GPL-2.0).

No source code or assets from either project are distributed in Hyprtia.

## NyarchAssistant companion

The optional `hyprtia-assistant` package builds NyarchAssistant at commit
`bd632c7148d54f7d7cb8a9290862c99e9f66e61e` and applies the separately
distributed StratOS patch in `packaging/nyarchassistant/stratos-minimal.patch`:
<https://github.com/NyarchLinux/NyarchAssistant>.

NyarchAssistant and the derivative patch are licensed under GPL-3.0-only. The
complete license text is included at `packaging/nyarchassistant/COPYING` and is
installed with the companion package. Hyprtia's MIT-licensed native shell and
the GPL companion remain separate programs and packages.

Orbitos Island at commit `3fde09b5438b4bf91208f942b0b7c9fe0ac60669`
is an architectural reference for local-first agent observability and bounded
background work:
<https://github.com/jomvick/Orbitos-island> (MIT).

No Orbitos source code or assets are distributed in Hyprtia.
