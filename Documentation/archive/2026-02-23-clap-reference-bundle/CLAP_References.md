Title: CLAP References
Document Type: Research Notes
Author: APC Codex
Created Date: 2026-02-21
Last Modified Date: 2026-02-21

Here are the main **CLAP (CLever Audio Plugin) related GitHub repositories** you’ll want to explore if you’re looking to build or use CLAP audio plugins (open-source, cross-platform plugin standard with an MIT license): ([GitHub][1])

**Core CLAP API & Tools**

* **[free‑audio/clap: Audio Plugin API (official CLAP SDK)](https://github.com/free-audio/clap?utm_source=chatgpt.com)** – Core CLAP format API + example templates to build a plugin. ([GitHub][1])
* **[free‑audio/clap-plugins: Example CLAP plugins](https://github.com/free-audio/clap-plugins?utm_source=chatgpt.com)** – Demo/example plugins using the CLAP API. ([GitHub][2])
* **[free‑audio/clap-juce-extensions: JUCE support for CLAP](https://github.com/free-audio/clap-juce-extensions?utm_source=chatgpt.com)** – Helpers to build CLAP plugins from JUCE projects. ([GitHub][3])
* **[free‑audio/clap-wrapper: Wrap CLAP into VST/AUv2/standalone](https://github.com/free-audio/clap-wrapper?utm_source=chatgpt.com)** – Allows packaging a CLAP plugin as VST3/AU or standalone. ([GitHub][4])

**Language & Binding Support**

* **[micahrj/clap-sys: Rust bindings for CLAP](https://github.com/micahrj/clap-sys?utm_source=chatgpt.com)** – Rust FFI bindings to the CLAP API. ([GitHub][5])
* **[JBetz/clap-hs: Haskell bindings for CLAP](https://github.com/JBetz/clap-hs?utm_source=chatgpt.com)** – Haskell support for CLAP development. ([GitHub][6])
* **[tobanteAudio/clap-examples: CLAP audio plugin examples in C++](https://github.com/tobanteAudio/clap-examples?utm_source=chatgpt.com)** – Additional examples showing how to write a CLAP plugin. ([GitHub][7])

**Cross-API Wrappers**

* **[Tremus/CPLUG: Wrapper for VST3, AUv2 & CLAP](https://github.com/Tremus/CPLUG?utm_source=chatgpt.com)** – Single C API wrapper that can target multiple plugin formats, including CLAP. ([GitHub][8])

**Community / Lists**

* **[RustoMCSpit/awesome-linux-clap-list (FOSS CLAP plugins)](https://github.com/RustoMCSpit/awesome-linux-clap-list?utm_source=chatgpt.com)** – A curated list of open-source CLAP plugins on GitHub. ([GitHub][9])

**How to approach building a CLAP plugin:**
Start with the **CLAP SDK (`free-audio/clap`)** for the API definition and sample templates. Then look at **`clap-plugins`** and **`clap-examples`** for concrete working code you can adapt. If you’re using a framework like JUCE, the **JUCE extensions repo** accelerates CLAP integration. If you prefer Rust or Haskell, bindings exist too. ([GitHub][1])

This ecosystem reflects a growing open standard meant to compete with proprietary formats like VST/AU, with the goal of simpler licensing and modern features such as advanced parameter modulation and genuine MIDI 2.0 support. ([en.wikipedia.org][10])

[1]: https://github.com/free-audio/clap?utm_source=chatgpt.com "free-audio/clap: Audio Plugin API"
[2]: https://github.com/free-audio/clap-plugins?utm_source=chatgpt.com "free-audio/clap-plugins"
[3]: https://github.com/free-audio/clap-juce-extensions?utm_source=chatgpt.com "free-audio/clap-juce-extensions"
[4]: https://github.com/free-audio/clap-wrapper?utm_source=chatgpt.com "free-audio/clap-wrapper"
[5]: https://github.com/micahrj/clap-sys?utm_source=chatgpt.com "micahrj/clap-sys: Rust bindings for the CLAP audio plugin ..."
[6]: https://github.com/JBetz/clap-hs?utm_source=chatgpt.com "JBetz/clap-hs: Haskell bindings for the CLAP audio plugin ..."
[7]: https://github.com/tobanteAudio/clap-examples?utm_source=chatgpt.com "tobanteAudio/clap-examples: CLAP Audio Plugin (C++)"
[8]: https://github.com/Tremus/CPLUG?utm_source=chatgpt.com "Tremus/CPLUG: C wrapper for VST3, AUv2, CLAP audio ..."
[9]: https://github.com/RustoMCSpit/awesome-linux-clap-list?utm_source=chatgpt.com "RustoMCSpit/awesome-linux-clap-list"
[10]: https://en.wikipedia.org/wiki/CLever_Audio_Plug-in?utm_source=chatgpt.com "CLever Audio Plug-in"
