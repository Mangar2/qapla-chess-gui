# qapla-chess-gui

## Build

Dieses Projekt verwendet [CMake](https://cmake.org) mit [Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) und [Ninja](https://ninja-build.org) sowie `clang++`.

### Voraussetzungen

- CMake ≥ 3.20
- Ninja
- clang++
- Git

### Klonen mit Submodulen

```bash
git clone --recurse-submodules https://github.com/<username>/<repo>.git
cd <repo>
```

```bash
cmake --preset default
cmake --build --preset default
```

```bash
cmake --preset release
cmake --build --preset release
```


---

### 2. 📦 **Abhängigkeiten / Submodules**

## Abhängigkeiten

Dieses Projekt nutzt folgende Bibliotheken als Git-Submodule:

- [GLFW](https://github.com/glfw/glfw)
- [ImGui](https://github.com/ocornut/imgui)
- [GLAD](https://github.com/Dav1dde/glad) (generiert, aber als Submodul enthalten)

Die Quellen liegen unter `extern/` und werden automatisch eingebunden.

## Lizenz

Dieses Projekt steht unter der [GNU GPL v3](LICENSE).  
Die genutzten externen Bibliotheken haben jeweils eigene Lizenzen:

- GLFW: zlib/libpng
- ImGui: MIT
- GLAD: MIT
