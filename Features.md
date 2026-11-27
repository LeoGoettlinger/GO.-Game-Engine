# GO. Game Engine – Features

**Stand: 28.10.2026 – Entwicklung, kein fertiges Release.**

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

## Aktueller Prüfstand

Der vollständige CI-Lauf `40649672758` hat für `6a5596e5` beide Editor-Builds und
Branding-Prüfungen bestanden. Die Templates liefen beim letzten Prüfen noch.
Neu hinzugekommen sind Kilo-HTTP-Client/Chat und die Projektanlage-Anbindung; diese
neueren Änderungen sind noch nicht durch einen vollständigen Editor-Build belegt.
Agenten-Toolausführung und Automatic Commit sind absichtlich deaktivierte Entwürfe.
Bestätigungsdialoge, Undo/Redo, asynchrone Git-Ausführung, Lifecycle-Reminder und
End-to-End-Tests fehlen noch. Frühere Aussagen „vollständig verdrahtet“ bedeuten
nicht, dass die Git Suite bereits alle Anforderungen erfüllt.

### Nachtrag: Original-Kilo und LFS-Modi

Das vollständige Kilo-Repository ist auf dem dokumentierten Commit geklont und
über Quelltext-Hashes geprüft. Die native Portierung ist **noch nicht vollständig**;
[Zuordnung zum Originalcode](docs/go/KILO_PORT.md). Die letzten Vollbuilds haben
Link-/Startfehler aufgedeckt; Korrekturen benötigen einen erneuten CI-Nachweis.

Zusätzlich angefragt, noch offen: LFS vollautomatisch nach Dateigröße oder
halbautomatisch für einen bzw. mehrere ausgewählte Ordner samt Unterordnern.

### Aktueller Stand: zwei zusätzliche LFS-Modi implementiert

Im Dialog **GO. Git LFS** gibt es jetzt neben den bisherigen Dateityp-Regeln:

| Modus | Verhalten |
| --- | --- |
| **Automatisch nach Größe** | Einstellbare MiB-Grenze; GO. erfasst passende Dateien beim Schreiben der Regeln und vor dem Staging. Kein Hintergrund-Watcher. |
| **Halbautomatisch nach Ordnern** | Einen oder mehrere Projektordner auswählen; rekursive Regeln gelten auch für neue Dateien darin. Außerhalb keine zusätzlichen GO.-Regeln. |

Text-/Godot-Dateien und Export-Artefakte bleiben ausgenommen. Einstellungen sind
lokal gespeichert, `.gitattributes` wird mit dem Projekt geteilt. Fremde Regeln
bleiben erhalten; bestehende Commits werden nicht umgeschrieben. Details und
Bedienung: [Git Suite](docs/go/GIT_SUITE.md#lfs-selection-modes-2026-10-28).

Die Implementierung und lokalen Regeltests sind vorhanden; visuelle Abnahme und
CI des neuen Stands stehen noch aus. Kilo-Toolausführung und Automatic Commit
sind weiterhin nicht freigeschaltet.

### Code Agentics: weiterer Implementierungsschritt

Der native HTTP-Client prüft jetzt strukturierte Tool-Aufrufe und zugehörige
Antwort-IDs. Ungültige oder unvollständige Gesprächsrunden werden vor dem Senden
abgewiesen. Dies ist eine getestete Protokollgrundlage, **noch keine freigeschaltete
Agenten-Ausführung**; Bestätigungen, Undo/Redo und Laufzeit-Abnahme fehlen weiterhin.

Die Quick Checks für den LFS-Stand `7c32b017` sind inzwischen einschließlich des
echten LFS-Binärtransfers erfolgreich. Der zugehörige Vollbuild lief beim Nachtrag
noch.

### Stand 30.10.2026: Git-Dock ohne blockierende Netzwerkaufrufe

Fetch, Pull, Push und Commit verwenden jetzt einen nichtblockierenden Git-Prozess
mit Zeitlimit, begrenzter Ausgabe und **Cancel Git**. Ein Abbruch macht bereits
abgeschlossene Änderungen nicht rückgängig; externe Git-Hilfsprozesse können
weiterlaufen. Statusabfrage, LFS-Dialog und Automatic Commit sind noch nicht
vollständig auf asynchrone Abläufe umgestellt.

Code Agentics hat jetzt strikte, pro Tool definierte Argumente und prüft sie vor
Editor-Zugriffen. Tool-Ausführung bleibt bis zu Bestätigungen, Undo/Redo und
Laufzeittests gesperrt. Die neuen nativen Prozesstests laufen mit dem nächsten
CI-Build; die vorherige Version `56ab458c` hat sämtliche Build-/Quick-/Stilprüfungen
bestanden. Die Gesamtanforderung ist weiterhin nicht vollständig abgeschlossen.

### Neuer Stand: experimenteller Szenen-Agent

Über **Scene tools (experimental)** lässt sich jetzt ein begrenzter Agentenmodus
aktivieren. Er liest Szenenbaum/Auswahl und kann einfache Nodes **hinzufügen oder
umbenennen**. Voraussetzungen: aktualisierter Modellkatalog und ein kostenloses
Modell mit nativer Tool-Unterstützung.

Änderungen werden standardmäßig einzeln bestätigt. Ablehnen und Stoppen sind auch
im Bestätigungsdialog möglich. Add/Rename verwenden die Godot-Undo-Historie;
Auto-Approve ist optional und wird bei einer neuen Konversation zurückgesetzt.
Pro Nutzeranfrage gilt eine Grenze von 20 einzelnen Tool-Aufrufen.

Das ersetzt die frühere Aussage „sämtliche Tool-Ausführung deaktiviert“ nur für
diese vier ausdrücklich freigegebenen Szenen-Tools. Dateiänderungen, Scripts,
beliebige Properties und Automatic Commit bleiben gesperrt. Die neuen nativen
Undo/Redo-Tests sind in CI eingebunden; Ergebnisse und visuelle Abnahme stehen
noch aus. Die vollständige Umsetzung von PROMPT.md ist damit weiterhin offen.

### Projekt-Erinnerungen und korrigierter Undo-Test

Die Git Suite prüft beim Öffnen aktivierter Projekte jetzt nach dem Editorstart
asynchron den Remote-/Git-Stand. Bei sauberem Stand erscheint kein Dialog; bei
Änderungen oder einem Rückstand gibt es eine Erinnerung mit Zugriff auf die Git
Suite und gegebenenfalls Pull. Ein vorhandenes Repository ohne aktivierte Suite
bekommt ein einmaliges lokales Aktivierungsangebot.

Beim Schließen folgt nach Godots Speicherabfragen ein abbrechbarer Git-/LFS-Check.
Danach kann man die Git Suite öffnen, abbrechen oder bewusst trotzdem schließen.
Die Erinnerungen lassen sich pro Projekt abschalten. **Automatic Commit ist hier
noch nicht verfügbar.** Die visuelle Abnahme dieser neuen Abläufe steht aus.

Der vorherige Vollbuild scheiterte im neuen nativen Undo-Test, nicht beim Kompilieren:
Der Test setzte irrtümlich einen vollständig laufenden Editor voraus. Die gemeinsamen
Operationen werden nun auf einer echten Core-Undo-History getestet; zur Laufzeit
bleiben sie an Godots Editor-History angebunden. Die Korrektur benötigt noch einen
erneuten CI-Nachweis.

### Automatic Commit: Controller weiter umgesetzt, noch nicht freigeschaltet

Die wichtigsten Git-Schritte des Controllers laufen jetzt asynchron: Diffs lesen,
Dateien stagen, neu normalisieren, committen und pushen. Ein Overlay-Lebenszyklusfehler
wurde korrigiert; neue native Tests prüfen die Freigabe der Eingabe bei simuliertem
Erfolg, Fehler und Abbau des Controllers.

Der Commit-Prompt behandelt sensible Dateinamen, Umbenennungen, mehrzeilige
Schlüssel und Binär-/Lockdateien konservativer. Status, Statistik und Diff haben
harte Größenlimits. Das ersetzt keine Prüfung eines Projekts auf vertrauliche Daten.

**Weiterhin gesperrt:** Der gesamte Automatic-Commit-Ablauf ist noch nicht
abgenommen. Auch Vorprüfung, LFS-Installation und Index-Pointerprüfung laufen
jetzt über asynchrone Git-Prozesse; die Größenprüfung verteilt Dateimetadaten auf
höchstens 16 Dateien pro Frame. Neue native Offline-Vollablauftests prüfen lokale
Commits, erfolgreiche/abgelehnte Pushes, LFS und die Lock-Freigabe. Deren
Windows-/Linux-Laufzeitergebnisse und die sichtbare Editor-Lock-Abnahme stehen aus.
Der vorherige Stand `6b09bc5d` hat inzwischen alle CI-Prüfungen bestanden.

### Eingabesperre über mehrere Editor-Fenster

Der Automatic-Commit-Controller sperrt jetzt sämtliche vorhandenen Viewports,
auch abgetrennte Fenster und SubViewports. Während des Vorgangs neu hinzukommende
Viewports werden ebenfalls gesperrt. Beim Entsperren wird je Objekt der vorherige
Zustand wiederhergestellt; zwischenzeitlich gelöschte Fenster werden sicher
übersprungen. Native Regressionstests dafür sind ergänzt; die sichtbare Abnahme
im echten Mehrfenster-Editor steht weiterhin aus.

### Code Agentics: projektlokaler Chat-Verlauf

Konversationen werden jetzt in `.godot/editor/go_chat_history.json` gespeichert.
**History** öffnet ein schreibgeschütztes Archiv; **Clear History** löscht es nach
Bestätigung einschließlich temporärer Dateien und Sicherung. Beim Neustart werden
keine alten Tool-Aktionen ausgeführt und keine Nachrichten automatisch versendet.

Der Verlauf ist nicht verschlüsselt. Grenzen: 20 Konversationen, 200 Einträge je
Konversation, 16.000 Bytes je Eintrag und insgesamt 512 KiB. Älteste Einträge werden
bei Erreichen der Grenzen entfernt. Beschädigte Dateien werden nicht still
überschrieben, Symlink-Pfade werden abgelehnt. Eine unterbrochene Ersetzung kann
beim nächsten Öffnen aus der vorherigen Sicherung wiederhergestellt werden.
Pure Tests für Format, Grenzen und Unicode bestehen lokal; native Speicher- und
Neustarttests sind für beide Plattformen in CI ergänzt (Laufzeitergebnis ausstehend).

### Automatic Commit im Schließen-Ablauf

Die Schließen-Integration ist jetzt implementiert: Der vorgemerkte Save-/Discard-
Entschluss bleibt erhalten, während der Commit läuft. Nur bei Erfolg wird dieser
Schließen-Ablauf fortgesetzt, nach Freigabe aller Eingabesperren. Bei Fehlern bleibt
der Editor offen; Abbrechen, Git Suite öffnen und ausdrücklich trotzdem schließen
bleiben möglich. Ein bereits laufender Git-Vorgang wird nicht zum Schließen beendet.
Die neue Aktion bleibt mit Automatic Commit zusammen gesperrt, bis die nativen
Vollablauf- und sichtbaren Editor-Tests bestanden sind.

### Code Agentics: weitere Szenenaktionen mit Undo/Redo

Der ausdrücklich aktivierbare Szenenmodus bietet jetzt acht Werkzeuge statt vier:
Baum/Selektion lesen, Nodes hinzufügen, umbenennen, entfernen und verschieben sowie
freigegebene Eigenschaften lesen/ändern. Entfernen erhält die Node-Objekte für Undo;
Ownership und ursprüngliche Reihenfolge werden wiederhergestellt. Verschieben
bewahrt den lokalen Transform und verhindert Zyklen und Namenskollisionen.

Eigenschaftsänderungen sind zunächst auf Klartext-Labeltext, Tooltiptext,
Sichtbarkeit, skalare Rotation und Z-Index begrenzt. Typen, Wertebereiche und
Skript-/Instanzgrenzen werden geprüft. Eine zwischen Bestätigungsdialog und
Anwendung geänderte Szenenhistorie oder ein ausgetauschtes Ziel wird abgelehnt.
Der Modus bleibt experimentell; neue native Undo-Tests und die sichtbare Abnahme
sind noch nicht als bestanden ausgewiesen. Datei-/Ressourcenwerkzeuge bleiben
weiterhin eine getrennte offene Aufgabe.

### Export- und Player-Prüfung in der vollständigen CI

Die Windows-/Linux-Pipeline prüft jetzt zusätzlich beide selbst gebauten Export-
Templates: Eine isolierte 2D-Testszene wird importiert, gestartet, als Debug und
Release exportiert und anschließend ohne Zugriff auf das Quellprojekt ausgeführt.
Fehler verhindern das Packaging; Prüfprotokolle bleiben als Artefakt erhalten.
Die neue Laufzeitprüfung ist noch ausstehend. Die Testszene wird vom Test angelegt,
nicht vom Agenten – die geforderte sichtbare Agent-/Player-Abnahme ist separat offen.

### Laufzeitprüfungen: Fehler konkret eingegrenzt

Ältere Linux-Vollbuilds einschließlich Editor und Templates sind inzwischen grün.
Ein neuer Undo-Test hatte jedoch fälschlich einen losgelösten Szenenbaum wie einen
geöffneten Editor-Szenenbaum behandelt; die Regression prüft jetzt den echten
SceneTree-Lebenszyklus. Windows-Automatic-Commit ist weiterhin nicht abgenommen.
Native Prüfungen verlangen nun vollständige XML-Ergebnisse und isolierte Caches;
fehlende Testergebnisse führen immer zu einem Fehler mit Diagnose, niemals zu
scheinbar erfolgreichen Tests.

### Code Agentics: Projektdateien lesen und durchsuchen

Der ausdrücklich aktivierbare **Project tools (experimental)**-Modus bietet jetzt
zusätzlich `list_files`, `read_file` und `search_text` an: insgesamt elf freigegebene
Werkzeuge. Lesen unterstützt Startzeile und Zeilenlimit. Such- und Leseausgaben
sowie Dateigröße und Verzeichnisdurchlauf sind begrenzt; unvollständige Suchen
werden als solche gekennzeichnet. Es werden keine Dateien geschrieben oder
Projekt-Scripte zum Lesen ausgeführt.

Symlinks, Git-/Editor-Interna, Windows-Gerätenamen und mehrdeutige Pfadvarianten
werden abgelehnt. Sensible Dateinamen und erkannte Zugangsdaten/Private Keys führen
zum Auslassen der gesamten Datei. Dieser heuristische Filter ist kein vollständiger
Secret-Scanner. Neue native Datei-/Symlink-Tests stehen in CI zur Laufzeitprüfung an;
lokale Guardrails, Git-Regression und Produktions-/Test-Syntaxprüfungen sind grün.

#### Dateiänderungs-Backend (experimentell)

Zusätzlich zu Godots normalen Editor-Funktionen ist eine vorbereitete
Dateiänderungs-API mit Inhaltsfingerabdruck, lokaler Sicherung und Schutz offener
Ressourcen vorhanden. Der Datei-Freigabedialog ist inzwischen angeschlossen;
die native Laufzeitabnahme und weitere Fehlertests stehen noch aus. Datei-Sicherungen ersetzen kein Undo/Redo.

#### Update: Datei-Freigabedialog im experimentellen Projektmodus

Der Projektmodus bietet jetzt 16 Werkzeuge inklusive fünf Dateioperationen.
Dateiänderungen benötigen immer eine eigene vollständige Vorher-/Nachher-Vorschau
mit Bestätigung; die Szenen-Autofreigabe umgeht diese nicht. Ablehnen und Stoppen
verwerfen die Freigabe. Native Tests für diese Abläufe sind ergänzt, die neue
Windows-/Linux-Laufzeitabnahme und visuelle Prüfung stehen noch aus.

#### Laufzeitstatus nach 13b24d8f

Der vollständige Linux-Lauf einschließlich nativer Datei-Freigaben und
Debug-/Release-Export mit Player-Start ist erfolgreich. Windows scheiterte am
LFS-Negativtest: Leere Prozessargumente gingen verloren, sodass die Testvorbereitung
den vorhandenen Filter abfragte statt deaktivierte. Beide Git-Prozessadapter
bewahren diese Argumente jetzt; neue native Regressionen prüfen den Effekt.
Die Windows-Bestätigung und visuelle Gesamtabnahme stehen noch aus.

#### Skript-Syntaxprüfung und lesender Zugriff auf Projekteinstellungen

Der experimentelle Agent verfügt jetzt über 18 aktive Werkzeuge.
`validate_script` prüft gespeicherten GDScript-Text mit dem Parser, ohne Skripte
zu laden oder auszuführen. Typ-/Abhängigkeits-/Laufzeitprüfung ist ausdrücklich
nicht enthalten. `get_project_setting` liest vier erlaubte String-Einstellungen;
beliebige Einstellungen und Objekte bleiben gesperrt. Native Tests sind ergänzt,
die neue Laufzeitbestätigung steht noch aus.

#### Neue Szenen und Ressourcen mit Freigabe

20 aktive experimentelle Werkzeuge: `create_scene` erstellt neue `.tscn`-Dateien
mit wählbarem erlaubtem Root-Typ und Namen; `create_resource` erstellt neue
`.tres`-Dateien aus acht einfachen Ressourcentypen. Beide nutzen die vollständige
Dateivorschau und eine separate Bestätigung, überschreiben keine vorhandenen
Dateien und öffnen keine Szene automatisch. Native Lade-/Instanziierungstests
sind ergänzt; die neue Laufzeitabnahme steht noch aus.

#### Player-Physik-Nodes und aktueller Build-Nachweis

`add_node` unterstützt zusätzlich `CharacterBody2D`, `CollisionShape2D`,
`CharacterBody3D` und `CollisionShape3D`. Die Erzeugung verwendet eine zentrale
Allowlist und den bestehenden Editor-Undo-/Redo-Pfad; beliebige Klassen und
Skripte bleiben ausgeschlossen. Ein neuer nativer Test prüft Parent/Owner nach
Anlegen, Undo und Redo. Die Laufzeitabnahme dieser Erweiterung steht noch aus.

Für den vorherigen Stand `c9960d2f` sind im Lauf `43481803336` Editor-Build,
Starttest und sämtliche nativen GO.-Tests auf Windows und Linux erfolgreich.
Templates und abschließende Export-/Player-Abnahme waren bei dieser Prüfung
noch nicht vollständig abgeschlossen. Das ist keine visuelle Gesamtabnahme.

#### Szenen öffnen und Projekt starten/stoppen (experimentell)

24 aktive Werkzeuge: `open_scene`, `run_project`, `run_current_scene` und
`stop_project` sind über eine separate Ausführungsfreigabe angeschlossen.
Szenen-Autofreigabe umgeht diese nicht. Vor Ausführung werden Projekt,
Szenendatei und Editorzustand erneut geprüft. Ungespeicherte Szenen und
Skript-/Textpuffer blockieren Öffnen/Starten. Der Dialog warnt ausdrücklich,
dass Projektcode ohne Sandbox mit den Benutzerrechten ausgeführt wird.

Ergebnisse melden die angeforderte Aktion, keinen garantierten Laufzeiterfolg.
Native Ablehnungs-/Bypass-Tests sind ergänzt; die tatsächliche Editor-Ausführung
und visuelle Gesamtabnahme dieser Erweiterung stehen noch aus.

### Automatic Commit activation after cross-platform runtime success

Full run 43511344381 for b21eef10 passed Windows and Linux editor, native suites,
templates and export/player gates. Automatic Commit is now registered while its
controller is in the scene tree and unregistered on exit/destruction. The dock
button is enabled for repositories after Refresh and disabled during Git work.
A separate confirmation explains staging/commit/push and anonymous Kilo diff
transmission with possible prompt logging and imperfect secret filtering. The
close reminder includes the same warning. Registry availability and close-flow
fixtures now exercise production activation instead of injecting a callback.
These activation/UI changes still require their own runtime/visual acceptance;
prior green results do not prove the new confirmation dialog visually correct.

### Guarded main-scene selection

`set_main_scene` is now the 25th experimental tool. Its required `path` must point
to a bounded readable res:// .tscn file. It serializes current ProjectSettings
with that main-scene override into a temporary local preview, without changing
project.godot or the in-memory main scene during preparation. The preview shows
all configuration differences, including other pending settings, and explicitly
warns that approving it saves those differences too.

The existing mandatory file confirmation, byte backup and replacement path apply.
The fingerprint also binds the target scene bytes and previous in-memory main
scene. A changed target, project configuration or pending settings invalidates
approval. The in-memory main scene is updated only after successful replacement.
It does not load/instantiate the scene or claim its dependencies are valid. Native
regressions cover no pre-approval mutation, stale target rejection, successful
configuration update and missing targets; runtime CI is pending.

### Guarded scene save requests

`save_current_scene` and `save_scene_as` are now enabled through mandatory editor
execution approval (27 tools total). Save As requires a new .tscn target unless
it is the current scene's own path; foreign existing scenes are never replaced.
Current-scene edits may be saved, but other unsaved scenes and script/text buffers
block the action. Project/scene/history/target bytes are fingerprinted, existing
bytes are backed up through the shared bounded backup helper, and state is
rechecked after backup before calling the editor.

The dialog warns about normal save callbacks and external-resource effects.
Godot's save_scene_as API returns no final save error, so the result deliberately
says save requested and directs the user to inspect editor errors. No success
claim is inferred from file existence. Native bypass/missing-editor tests now
cover both save actions; actual save UI/round-trip acceptance remains pending.

### Script attachment through explicit execution approval

`attach_script` is enabled (28 tools), with required `script_path` (.gd) and
optional scene-relative `node_path` (default root). It accepts only local nodes in
non-inherited scenes that do not already have a script, so existing script state
is not discarded. Unsaved script buffers, foreign targets and stale approvals
are refused. Approval explicitly warns that resource loading can execute static
initialization/tool scripts; there is no sandbox or rollback of those effects.

After approved loading, the bridge revalidates and re-resolves the target, checks
script validity and native base compatibility, and records assignment through the
editor undo history. The old direct set_script draft was removed. A native test
checks resource retention across assignment/undo/redo; denial tests cover missing
editor/approval. Full actual-editor attachment acceptance is still pending.

### Resource assignment for player components

`assign_resource` adds a 29th tool: required res:// .tres `path`, required
`property`, optional scene-relative `node_path`. Supported targets are
Sprite2D.texture (Texture2D) and CollisionShape2D/3D.shape (Shape2D/3D). Local
non-scripted targets only; existing resource assignments are retained for undo.
The approved target, resource bytes, old resource identity and history are checked
again after loading. Loading can execute dependency scripts and therefore always
requires explicit execution approval, not scene auto-approval. Only assignment,
not arbitrary initialization effects, is undoable. GradientTexture2D is now also
allowed by create_resource. Native shape-reference undo/redo and bypass tests are
added; real editor assignment and visual validation remain pending.

### Scene instantiation with approved initialization

`instantiate_scene` adds a 30th tool: required .tscn `path`, required unique ASCII
`name`, optional local parent `node_path`. It loads/instantiates only after an
explicit execution warning and then rechecks parent/history/file state before
inserting through UndoRedo. The parent must be locally owned and non-scripted;
inherited scenes and name collisions are refused. Instances over 1000 nodes are
rejected after creation; this is not a pre-allocation sandbox. Loading and script
initialization effects are not undone. Nested scene ownership is preserved by the
shared insertion path and covered by a new native undo/redo fixture. Real-editor
instantiation acceptance still needs the new full CI run.

### Actual-editor tool-chain and graphical evidence gate

The full build now creates its export fixture through production tool-bridge
calls inside a running editor, rather than writing a prebuilt player scene.
The test creates/opens a CharacterBody2D scene, adds sprite/collision nodes,
checks real editor Undo/Redo, creates/assigns resources, writes/validates/attaches
movement code, saves the scene and sets it as main. It waits for import/save
completion and checks persisted output before emitting GO_AGENT_SCENE_OK.
Both debug and release exports then run independently of the source project and
must emit GO_PLAYER_SMOKE_OK without editor classes.

The deterministic test hook is compiled only with GO_SUITE_TESTS_ENABLED and
requires --go-agent-acceptance, the exact fixture project name and a marker file.
Existing player/shape/texture files abort the test; no live gateway request is
made. Test approval deliberately invokes the prepared bridge API: it is NOT a
human-clicked dialog test and NOT proof of live AI tool planning.

On Linux, a subsequent Xvfb run opens the actual editor/player scene and captures
editor-ui.png plus its log. Generated project files and logs are retained in the
GO-validation artifact. Capture requires a visible X11 window but is NOT manual
visual review. Windows graphical review and live-AI acceptance remain open.
All these new acceptance gates are implemented but await their first full CI
execution; mocked orchestration tests are not a substitute for that result.

### Remaining project controls and asynchronous agent Git status

36 tools are now enabled. `scan_filesystem`, `select_node`, `auto_commit` and
`get_scene_tree` use mandatory editor-action approval. Selection is bounded to
the open scene and rechecks after clearing the previous selection. Scene-tree
inspection loads a saved .tscn resource without instantiating it, returns at most
1000 declarations/48000 bytes, and does not expand inherited/instanced subscenes.
Import/resource loading may execute project callbacks and is explicitly not a
sandbox. Agent Automatic Commit uses the same registered controller and lock as
the dock and close reminder, with an explicit gateway/privacy/stage/push warning.

`set_project_setting` accepts only name=application/run/main_scene and a scene
string value; it delegates to the backed-up full settings-preview flow. Other
setting writes are refused. Preview serialization now restores Godot's implicit
feature-list normalization until approval, and applies that list only after the
configuration replacement succeeds.

`git_status` runs nonblocking with a 30-second timeout and owns its reservation.
Stop/exit cancels only its own job. The response contains a local changed-path
count, not filenames/content, remote-fetch state or LFS-transfer claims. A native
dock test covers actual local Git, completion, privacy and cancellation ownership.
New editor-action denial tests and setting allowlist checks are added. Runtime
acceptance remains pending. The UI capture script executable-bit style failure
from dc8e0d90 is corrected in this checkpoint.

### Explicitly reviewed editor diagnostics

`get_editor_log` completes the current 37-tool whitelist. A small registered
upstream C++ adapter exposes the entire retained editor-output buffer only when
it fits 48000 UTF-8 bytes. Larger buffers are refused, not partially tailed. The
bridge screens the snapshot for possible credentials and then requires explicit
approval showing exactly that text before sharing it with the selected model.
The snapshot hash is rechecked at execution. Automatic screening is imperfect;
private paths/data may remain, so review is mandatory. Evicted history, separate
debugger state and repetition counts are not reconstructed.

The actual-editor acceptance fixture now exercises the log tool, then verifies
whole-buffer size rejection and a known secret-marker rejection. The upstream
adapter is documented in UPSTREAM_PATCHES.md and included in real-header syntax
checks for production/test layouts. Runtime and visual confirmation are pending.

### Offene Paket-Lücken aus dem Abschlussaudit

Die 37 angeschlossenen Agentenwerkzeuge bedeuten noch keine Gesamtfreigabe.
Die Offline-Verwaltung der Plugins im geöffneten Projekt ist noch ein deaktivierter
Platzhalter. Sieben GDScript-Addons sind vorhanden; Dialogic und die vier
GDExtension-Binärpakete fehlen noch. Ein getestetes Standard-Plugin-Set, aktuelle
Windows-/Linux-Laufzeitabnahmen, Screenshots und der Live-KI-Durchlauf stehen aus.

### Offline plugin management implementation checkpoint

The earlier disabled-plugin-manager blocker is now replaced by an offline catalog
in the Git Suite. Install copies to a staging folder, refuses existing addon
folders and linked paths, and never downloads, overwrites or deletes user files.
Enable/disable requires explicit confirmation; enabling refuses a currently
enabled conflicting dialogue addon. Disabling retains files. Failures retain
staging data for inspection. The manager refuses operations during Git/editor
locks, project execution or filesystem scanning. Import and plugin activation
execute third-party code and are not sandboxed or scene-undoable.

Dialogic 2 alpha-20 is now source-vendored at the existing pinned commit with
SHA-256 provenance and unchanged MIT license; eight GDScript addons are bundled.
New-project copying includes Dialogic, but activation remains opt-in pending
actual startup/compatibility checks. Native path-safety regression coverage is
added. The dialog itself and Dialogic startup still require runtime/visual
acceptance. Four GDExtension binary bundles remain outstanding.

### Checksum-pinned extension packaging gate

All four GDExtension release assets now have fixed upstream URLs and SHA-256
values recorded from GitHub release metadata. A maintainer-only CI bundler
verifies the digest before safe extraction, rejects traversal/links/ambiguous
addon roots and requires a target-platform library. Upstream licenses fetched
from the pinned source commits are included. Packaging merges only bundles with
matching provenance and reports them separately from source-vendored addons.
A short dedicated workflow validates Windows/Linux archive layout without an
engine rebuild; the full release job runs the same bundler before packaging.

Five offline archive tests cover checksum rejection, traversal, ambiguous roots,
missing target binaries and no-overwrite behavior. Local real downloads are
blocked by the sandbox's GitHub release-CDN restriction, so actual archive
layout/download and native startup acceptance are pending CI. LimboAI uses its
published GDExtension 4.6 artifact, not a replacement upstream editor; compatibility
with GO. 4.7.2 must still be checked. GDExtensions without plugin.cfg are not
currently installable through the GDScript plugin dialog. No binary is committed.

Dialogic's upstream automatic updater was also disabled: its check/download
methods return unavailable/failure without HTTP requests. This additive source
patch is recorded in PATCHES.md and its provenance; pinned bundles must not
self-update when opened offline.

### Native-only addon installation

The offline manager now also recognizes root `.gdextension` descriptors without
`plugin.cfg`, allowing verified release addons such as Godot Git Plugin to be
copied. Consent explicitly warns that import loads native machine code. Native
extensions without an EditorPlugin switch cannot be disabled/unloaded from this
dialog; no unsafe file deletion or unload is attempted. For addons with both,
disabling the editor plugin does not unload the native library. Native startup
acceptance and the actual UI interaction remain pending.

### Archive CI result and plugin startup gate

The corrected pinned-extension workflow for 76dd5a12 has passed Linux and Windows
archive verification and artifact upload (run 46042242506). The Git addon uses
upstream's `addons/godot-git-plugin` directory, not the earlier underscored guess.
This proves checksum/layout/library presence, not ABI or editor compatibility.

The full build now starts every bundled addon separately in a disposable project
using the compiled GO. editor, first importing, then enabling its EditorPlugin
where present. Native-only extensions load during import. Errors, timeout and
nonzero exits block release packaging; per-plugin logs are preserved. This new
startup gate has not completed yet. Defaults remain disabled until startup and
visual/interaction checks pass. An editor starting successfully does not by
itself prove every plugin feature or exported-project behavior.

### Guarded JSON fallback execution

The previously preview-only JSON fallback now runs through the same project-tool
permission snapshot, 20-action budget, argument/path checks and mandatory write/
execution approval pipeline as native calls. Chat-only requests still cannot
execute actions. Models without native support receive the enabled schemas and
single-object JSON instructions; the internal validated transcript is converted
to ordinary assistant/user messages with explicitly untrusted tool-result data,
not unsupported native tool roles. Synthetic call IDs are unique across rounds.

Standalone tests cover transcript conversion and orphan-result rejection. A
native dock regression checks opt-in, unapproved writes, denial, unique IDs and
the action cap. Local syntax and standalone checks pass; new native/live runtime
results remain pending. This supersedes earlier statements that fallback actions
are never executed.

### Anonymous gateway planning probe

A separate explicitly synthetic live Actions workflow now requests the model
catalog and at most one chat completion from the fixed Kilo Gateway using the
public anonymous credential. Only the production default model is accepted and
only when the production catalog parser verifies it is free. The fixed prompt
contains no repository files, diffs, user paths, credentials or environment data.
Redirects, oversized responses, paid/authenticated fallback and automatic retries
are refused. A generated `list_files` plan is checked using the production C++
response parser and tool argument/path schema, but is NOT executed in an editor.
Artifacts record only status and scope, not arbitrary gateway response bodies.

Three offline tests compile the real C++ probe while mocking HTTP: valid free
planning, refusal to send chat for a paid/missing default model, and rejection of
an unexpected action. Those pass locally. Live result and full live-agent editor
acceptance remain separate and pending; this probe does not claim user-clicked
approvals, player creation or screenshot acceptance.
