# GO. Game Engine – Features

**GO. Game Engine** ist ein Fork von Godot `4.7.2-stable`. Ziel ist nicht, Godot zu ersetzen,
sondern den Editor um zwei fest eingebaute Suites, ein eigenes Branding, ein Offline-Plugin-Paket
und einen automatisierten Commit-Ablauf zu erweitern. Alle Godot-Funktionen bleiben erhalten.

> **Ehrlicher Status:** Viele Bausteine sind als Quelltext, Guardrail-Logik und CI-Checks
> vorhanden. Ein vollständig kompilierter GO.-Editor mit Screenshots ist noch nicht gebaut.
> Details stehen in [`docs/go/ROADMAP.md`](docs/go/ROADMAP.md).

## Legende

- ✅ **vorhanden**: Quelltext/Checks im Repo, lokal oder in CI automatisiert geprüft.
- 🧩 **teilweise**: Logik/UI-Quelltext vorhanden, aber noch nicht im kompilierten Editor bewiesen.
- 🚧 **geplant**: Im Prompt spezifiziert, aber noch nicht fertig verdrahtet.

---

## 1. Zwei fest eingebaute Editor-Suites 🧩

Normales Godot kennt Docks, aber keine mitgelieferte Git-Suite und keinen nativen KI-Agenten.
GO. registriert unter `modules/go_suite/` ein **Editor-only-Modul** mit zwei Docks:

| Dock | Zweck | Im normalen Godot | In GO. |
| --- | --- | --- | --- |
| **Git Suite** | Status, Commit, Pull, Push, Remote-Assistent, LFS | höchstens über Addons/GDExtension | fest eingebaut, host-neutral |
| **Code Agentics** | Chat mit freien Kilo-Modellen, Godot-Tools | nicht vorhanden | nativer C++-Port, anonym, free-only |
| **GO. Suites** | Launcher zum Ein-/Ausblenden der Docks | nicht vorhanden | drittes Dock mit Icons |

Die Docks haben eigene SVG-Icons im Godot-Stil und sind über den Launcher rechts ein-/ausblendbar.
Icons, Registrierung, reine C++-Logik, Guardrails und Doctests sind ✅; der kompilierte Editor
mit Screenshots ist 🧩.

---

## 2. Host-neutrale Git Suite ✅/🧩

Normales Godot setzt kein `origin` voraus. GO. enthält eine Git Suite, die das System-`git`
mit `-C <projekt>` aufruft und **keine Passwörter oder Tokens speichert**.

- ✅ Remote-Parser für **GitHub, Codeberg/Forgejo, GitLab, Custom, Local**.
- ✅ Erkennung von HTTPS, `ssh://`, SCP-Style-SSH, Ports, Owner/Repo und Web-URLs.
- ✅ Abweisung von URLs mit eingebetteten Credentials oder Query-Tokens.
- ✅ Reiner C++-Command-Builder und Ausgabe-Parser für Status, Branch, Ahead/Behind, Log,
  Diff, LFS-Dateien, LFS-Status und Pointer-Präfixe.
- ✅ Lokaler Bare-Repo-Test: `init → commit → push → clone → Status`, ohne Netzwerk.
- 🧩 Remote-Assistent mit Host-Auswahl, Protokoll-Umschalter, URL-Vorschau und
  „Repository im Browser anlegen“-Button (Quelltext + Syntax-Check vorhanden).
- 🧩 Status, Stage, Diff, Log, Commit, Pull, Push und Fetch im Dock verdrahtet
  (Quelltext + Syntax-Check vorhanden, Editor-Screenshot steht noch aus).

**Wichtig:** Codeberg ist gleichwertig zu GitHub/GitLab; LFS wird nicht an GitHub-only-Code
gekoppelt. Authentifizierung bleibt beim Credential Helper bzw. SSH-Agent des Systems.

---

## 3. Git LFS von Anfang an ✅/🧩

Godot legt bei „Version Control Metadata: Git“ bereits `.gitattributes` an, aber ohne LFS.
GO. wird LFS-Regeln **vor dem ersten Stage** setzen:

- ✅ Verwalteter `.gitattributes`-Block mit `*.png`, `*.jpg`, `*.wav`, `*.ogg`, `*.mp4`,
  `*.ttf`, `*.otf`, `*.zip`, `*.blend`, `*.fbx`, `*.glb`, `*.bin`, `*.res`, `*.scn` usw.
- ✅ Textformate wie `.gd`, `.tscn`, `.tres`, `.gltf`, `.svg`, `.json`, `.cfg`, `.godot`
  bleiben normale Git-Dateien.
- ✅ Große-Dateien-Klassifizierung: Warnung ab 5 MiB, rot ab 50 MiB, harte GitHub-Grenze
  100 MiB ohne LFS.
- ✅ LFS-Pointer-Erkennung am Dateianfang (`version https://git-lfs.github.com/spec/v1`).
- ✅ Command-Builder für `git lfs version/install/track/pull/push/ls-files/status/env/migrate`
  sowie explizite `pkexec`-/`winget`-Installationskommandos.
- ✅ Bare-Repo-Test mit verwalteter `.gitattributes`, 6-MiB-Großdatei-Klassifizierung und
  synthetischem LFS-Pointer im Index.
- 🧩 Dialog mit Check, System-Installation, `git lfs install --local`, Regeln schreiben,
  Große-Dateien-Tracking, Download/Upload, Pointer-Verifikation im Index und Quota-Anzeige
  (Quelltext + Syntax-Check vorhanden).
- 🚧 Echte `git lfs`-Binär-Installation auf Windows/Linux im kompilierten Editor testen.

Bereits committete Blobs werden **nie ungefragt** migriert; `git lfs migrate` bleibt ein
optionaler, mehrfach warnender Assistent.

---

## 4. Automatic Commit 🧩/🚧

Normales Godot hat keinen eingebauten „Alles sichern und pushen“-Knopf. GO. wird einen
deterministischen Ablauf bieten:

1. Vorprüfung: Repo, Änderungen, Merge-Konflikte, `user.name`/`user.email`.
2. **Vollbild-Overlay sperrt die komplette Editor-UI** inklusive Fenster-Schließen.
3. Sechs Etappen mit Progress: Änderungen lesen → Commit-Message → Staging → Commit → Push → Fertig.
4. Commit-Message über **nur `kilo-auto/free`** aus einem hart gekürzten, secret-bewussten Diff.
5. Fallback `chore: update project files`, wenn Kilo offline ist oder einen Fehler liefert.
6. Bei jedem Erfolg **und jedem Fehler** wird die UI garantiert wieder freigegeben.

- ✅ Zustandsmaschine und Unlock-Invariant unit-getestet.
- ✅ Commit-Message-Sanitisierung und secret-aware Diff-Kürzung unit-getestet.
- 🧩 Overlay/Progress/Fehlerpfade als Godot-UI-Quelltext geplant bzw. in Arbeit.
- 🚧 Verdrahtung mit System-Git, Kilo-HTTP-Client, Schließen-Dialog und `auto_commit`-Tool.

Kein Force-Push, keine automatische Konfliktlösung, keine Endlosschleife.

---

## 5. Code Agentics – Kilo Free nativ im Editor 🧩

Normales Godot hat keinen KI-Assistenten. GO. portiert das Verhalten von Kilo Code nativ nach
C++/Godot, ohne Node.js, ohne VS Code, ohne API-Key-UI.

- ✅ Anonymes Gateway `https://ai-gateway.kilo.ai/api/v1` mit `Bearer anonymous`.
- ✅ Free-Filter: nur `kilo-auto/free`, `isFree == true`, `:free`-IDs oder 0/0-Pricing.
- ✅ 429/`Retry-After`-Auswertung gegen einen lokalen Mock-Server getestet.
- ✅ Whitelist für Godot-Tools und strikte `res://`-Pfad-Guardrails.
- ✅ Verbotene UI per Source-Scanner abgesichert: kein Provider, API-Key, Login, BYOK,
  Billing, MCP, Workspace-Picker, Folder-Picker, Terminal oder Shell-Tool.
- 🧩 Chat-Dock mit locked workspace `res://`, Auto-Approve-Schalter, Stop, Verlauf, About.
- 🚧 Echter `HTTPRequest`-Client, 20-Schritt-Tool-Loop, Undo/Redo und Player-Smoke-Test.

**Privacy:** Free-Modelle können Prompts beim Anbieter loggen. Keine Keys, Tokens oder
persönlichen Geheimnisse in den Chat eingeben.

---

## 6. Bundled Plugins und Offline-Projektanlage ✅/🚧

Normales Godot lädt Plugins bei Bedarf aus der Asset Library. GO. liefert ein **offline**
verfügbares Paket mit:

- ✅ 12 MIT-geprüfte Plugin-Pins in `go_bundled_plugins/plugins.json`.
- ✅ 7 GDScript-Addons quellvendort, mit Tag + 40-Zeichen-Commit + SHA-256-Tarball.
- ✅ Reproduzierbare Vendor-/Validierungs-Skripte und 397 GDScript-Dateien in CI geparst.
- ✅ `go_package_release.py` erzeugt ZIP, Godot-kompatibles `.tpz` und SHA-256-Checksums.
- 🚧 GDExtension-Binaries (Terrain3D, LimboAI, Git Plugin, Debug Draw 3D) per CI bündeln.
- 🚧 Neues Projekt mit Checkbox `Enable GO. Git Suite …`, `git init -b main`, Plugin-Kopie,
  GO.-Boot-Splash und `go/git_suite/enabled=true`.
- 🚧 Offline-Aktivierung/Deaktivierung bereits geöffneter Projekte über `GO. → Bundled Plugins`.

---

## 7. Branding, Splash und Version ✅

| Feature | Normales Godot | GO. |
| --- | --- | --- |
| Produktname | Godot Engine | **GO. Game Engine** |
| `short_name` | `godot` | `go` |
| Versionszeile | `Godot Engine v4.7.2…` | `GO. Game Engine v4.7.2.stable.<build>` |
| Editor-Splash | Godot-Standard | GO.-Splash mit exaktem Text unten rechts |
| Boot-Splash | Godot-Standard | `A Godot Game Engine Fork by SampleCraft` |
| Font | Godot-UI-Font | Monocraft (OFL-1.1) für GO.-Eigen-UI |

- ✅ `version.py`, `--version`, About, Projekt-Manager, README, App-Icon.
- ✅ Reproduzierbare PNG-Quellen unter `go_assets/rendered/` und Generator `make_splash.py`.
- 🚧 Screenshot des laufenden Editors mit Splash.

Godot-Credits, `LICENSE.txt`, `AUTHORS.md`, `DONORS.md` und das Upstream-README bleiben
erhalten. GO.-Code ist MIT, Monocraft OFL-1.1, Kilo-Konzepte MIT mit Hinweis.

---

## 8. CI, Release und Sicherheit ✅/🧩

- ✅ `.github/workflows/go-quick-checks.yml`: Branding, Splash-Reproduzierbarkeit,
  Plugin-Manifest, YAML, C++-Guardrails, UI-Scanner, Kilo-Mock, lokaler Bare-Repo-Git-Flow,
  echte Godot-Header-Syntaxprüfung aller 23 Modul-C++-Dateien, Unit-Tests, Packaging-Smoke
  und echtes Godot-4.7.2-GDScript-Parsing.
- ✅ `.github/workflows/go-build.yml`: Windows + Linux Editor-/Template-Builds und Release-ZIPs
  auf Tag/`workflow_dispatch`.
- ✅ Geerbte Upstream-Static-Checks laufen auf dem GO.-Branch; die fremde Full-Matrix
  (Android/iOS/Web/macOS/Mono) ist auf diesem Branch abgeschaltet.
- 🧩 Vollständiger C++/SCons-Editor-Build in CI noch nicht grün bewiesen; der Syntax-Check
  ersetzt ihn ausdrücklich nicht.
- ✅ Keine Telemetrie, keine Secrets im Repo, keine echten Gateway-Calls in Tests.

---

## 9. Was bewusst **nicht** kommt

- Keine C#/.NET-Unterstützung in diesem Fork.
- Kein macOS-Signing/Notarisation.
- Keine kostenpflichtigen Kilo-Modelle, keine Accounts, keine Provider-Verwaltung.
- Keine stillen System-Installationen, keine Passwort-/Token-Speicherung.
- Kein ungefragtes Umschreiben bestehender Git-Historie.
- Kein kaputtes Godot: Export-Templates und Projekte bleiben Godot-kompatibel.

---

## Short English summary

GO. is a Godot 4.7.2-stable fork by SampleCraft. Compared with upstream Godot it adds a
built-in editor-only Git Suite, a native anonymous Kilo free-model chat dock, an Automatic
Commit flow with a full UI lock, reproducible GO. branding/splashes, an offline MIT plugin
bundle, and dedicated Windows/Linux CI/release packaging. The source, pure guardrails, system-Git
command builder/parsers, local bare-repo flow, LFS pointer test, mock gateway tests, plugin
validation and fast CI checks are implemented. All 23 Godot-facing module C++ files pass a
real-header syntax check. A fully compiled editor, real `git lfs` binary smoke test, Kilo HTTP
client, tool loop and UI screenshots are still open and tracked honestly in
`docs/go/ROADMAP.md`.

---

**Weiterführende Dokumentation**

- [`README.md`](README.md) – Einstieg auf Deutsch
- [`docs/go/ARCHITECTURE.md`](docs/go/ARCHITECTURE.md) – Modulaufbau
- [`docs/go/GIT_SUITE.md`](docs/go/GIT_SUITE.md) – Git Suite
- [`docs/go/CODE_AGENTICS.md`](docs/go/CODE_AGENTICS.md) – Code Agentics
- [`docs/go/PLUGINS.md`](docs/go/PLUGINS.md) – Plugin-Katalog
- [`docs/go/ROADMAP.md`](docs/go/ROADMAP.md) – offener, ehrlicher Status
