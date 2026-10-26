Hallo,

## PROMPT-ANFANG

Du bist ein autonom arbeitender Coding-Agent in einem frischen, echten Fork der Godot Engine (`godotengine/godot`). Bau daraus die neue Engine **„GO. Game Engine“**. Arbeite eigenständig, in kleinen überprüfbaren Commits, bis alle untenstehenden Akzeptanzkriterien erfüllt sind. Stelle nur Rückfragen, wenn eine Entscheidung wirklich nicht aus diesem Prompt ableitbar ist – alle wichtigen Default-Entscheidungen stehen bereits unten.

### 0. Produktvision

- Die Engine heißt überall **GO. Game Engine** (mit Punkt). Das ist ein Wortspiel: „GO.“ + „dort“ spricht man wie „Godot“.
- GO. ist ein **echter Full-Fork von Godot**: Der Godot-Quellcode liegt bereits in diesem Repo. Branding und neue Funktionen werden direkt im Godot-Code geändert bzw. als fest einkompilierte Editor-Features ergänzt – nicht nur als loses Projekt-Addon, das man pro Spiel nachinstallieren muss.
- Es gibt genau **zwei neue Funktionssuites**:
  1. **GO. Git Suite** – vollständige Git-Oberfläche im Editor.
  2. **Code Agentics** – die Agentic-Coding-Suite, ein umgebrandeter/nativ nachgebauter Kilo-Code-Agent direkt im Godot-Editor.
- Beide Suites sind über eigene Buttons in einem **Seitenmenü/Dock** im Editor zu öffnen (wie andere Godot-Docks, andockbar, mit Icon, jederzeit ein-/ausblendbar).
- Zusätzlich sind die bekanntesten, besten und nützlichsten Godot-Plugins **fest mitgeliefert** und stehen in neuen Projekten ohne Download/Installation zur Verfügung (Details §10).
- Beim Erstellen eines neuen Projekts kann man wählen, ob die **gesamte Git Suite für dieses Projekt aktiviert** wird (das Projekt ist dann ein Git-Repository, Details §7).
- Beim Öffnen und Schließen eines Projekts erinnert GO. daran, Änderungen zu committen/pushen bzw. die neuesten Änderungen zu pullen (Details §7).
- Auf dem Lade-/Splash-Screen steht **unten rechts** exakt dieser Text in einer Minecraft-Font:
  `A Godot Game Engine Fork by SampleCraft`

### 1. Nicht verhandelbar (harte Anforderungen)

1. **Name/Branding:** `GO. Game Engine` in Fenster-Titel, Projekt-Manager, About-Dialog, `--version`-Ausgabe, Splash und Release-Artefakten. Godot-Credits/Lizenzen bleiben erhalten (es ist ein Fork, keine Fremdausgabe als Eigenwerk); im About-Dialog/README muss stehen: „GO. Game Engine is an unofficial fork of the Godot Engine“.
2. **Splash-Text:** exakt `A Godot Game Engine Fork by SampleCraft`, unten rechts, gut lesbar, in Minecraft-Pixel-Font (Lizenz-Regel §3.3). Sowohl auf dem **Editor-Splash** (Ladescreen der Engine) als auch im Standard-Boot-Splash neuer GO.-Projekte.
3. **Code Agentics = nur Kilo Free, sonst nichts:**
   - **Keine** Möglichkeit, Provider hinzuzufügen. Keine API-Key-Felder, kein „Custom Provider“, kein OpenAI-/Anthropic-/OpenRouter-Menü, keine versteckte Einstellung dafür.
   - **Kein Kilo-Account-Login.** Ausschließlich die normalen Gratis-Modelle von Kilo über das Kilo Gateway, anonym. Default-Modell: `kilo-auto/free` (der Free Router).
   - **Keine MCP-Server**: keine UI, keine Config, kein Codepfad zum Hinzufügen von MCP-Servern. Was aus Kilo als Referenz übernommen wird, muss MCP-frei sein.
   - **Workspace ist fest und unveränderlich der Projektordner** (`res://`) des gerade geöffneten Projekts. Kein Verzeichniswechsel, kein Folder-Picker, kein „Open Folder“, keine Pfade außerhalb `res://` (Pfad-Traversal mit `../`, Symlinks, absolute Pfade etc. technisch ablehnen).
   - Der Agent kann **nicht nur Dateien**, sondern **Godot selbst** bedienen (Szenen, Nodes, Properties, Scripte, Ressourcen, Projekt starten/stoppen, Undo/Redo usw., komplette Tool-Liste §9).
4. **Git Suite mit „Automatic Commit“:** Der Button heißt exakt **`Automatic Commit`**. Beim Drücken wird über Kilo (Free Router) eine gute Commit-Message aus den Änderungen erzeugt und der Commit + Push ausgeführt. Während des ganzen Vorgangs ist die **gesamte UI gesperrt** (modales Overlay + Progress Bar), damit man während Commit/Push nichts editieren/kaputt machen kann. Nach Erfolg **oder** Fehler wird die UI garantiert wieder freigegeben (§8).
5. **Erinnerungen:** Bei aktivierter Git Suite: Erinnerung beim **Öffnen** (neue Commits auf Remote? uncommittete Änderungen?) und beim **Schließen** (alles committed? alles gepusht?) (§7).
6. **Plugins ohne Nachinstallation:** Gebündelte Plugins liegen im Fork/im Release und werden bei neuen Projekten mitkopiert & (soweit sinnvoll) auto-aktiviert. Kein Laufzeit-Download aus der Asset Library nötig. Standard-Betrieb (Editor öffnen, Projekt anlegen) muss offline funktionieren (außer Code Agentics / Git-Netzwerkfunktionen selbst).
7. **Kein kaputtes Godot:** Der Fork muss weiterhin als Godot-Editor bauen und Projekte exportieren können. Upstream-Sync (Rebase/Merge von `godotengine/godot`) muss machbar bleiben: Eigene Änderungen an Upstream-Dateien minimal und mit `// GO.`-Kommentar markieren; neue Funktionen so weit möglich in eigenen Modulen/Dateien.
8. **Git-Hosts gleichwertig: GitHub, Codeberg, GitLab und generisches Git.** Die Git Suite ist host-neutral (normales Git über SSH/HTTPS), erkennt den Host am `origin` (insbesondere **Codeberg** `codeberg.org`, Forgejo) und bietet einen Remote-Einrichtungs-Assistenten mit Host-Auswahl an, statt GitHub vorauszusetzen (§6.1). GO. fragt/speichert dabei niemals Passwörter oder Access-Tokens; Authentifizierung bleibt Sache des System-Git (Credential Helper/SSH).
9. **Git LFS wird automatisch eingerichtet, damit große Dateien von Anfang an richtig laufen** (§6.2): Erkennung ob Git LFS installiert ist, geführte Ein-Klick-Installation falls nicht (mit Admin-/UAC-Bestätigung des Users, wo das OS sie verlangt – niemals still im Hintergrund Systempakete verändern), `git lfs install` global + im Projekt, Godot-sinnvolle `.gitattributes`-LFS-Regeln **vor** dem ersten Stage großer Assets, automatische Erkennung großer/nicht getrackter Binärdateien vor Commit/Automatic Commit, LFS-Download beim Öffnen/Pullen und LFS-Upload beim Push. Bereits normal committete Historie wird **nie ungefragt** per `git lfs migrate` umgeschrieben.

### 2. Defaults (nicht mehr nachfragen)

- **Basis:** Wenn dieses Repo auf dem Godot-`master` (Dev) steht, erst `upstream` (`https://github.com/godotengine/godot.git`) einrichten, fetchen und die GO.-Arbeit auf der neuesten stabilen 4.x-Version aufbauen. Stand der Recherche: **`4.7.2-stable`** (Tag `4.7.2-stable`, Commit `ed1daf0bf001b61586d9930840f2f1394092c079`). Prüfe kurz, ob es inzwischen ein neueres stabiles 4.x gibt, und notiere die gewählte Basis (Tag + Commit) in `docs/go/BASE_VERSION.md`. Ohne Anweisung keine Godot-3-Basis und kein Wechsel auf eine Dev-Version mitten in der Arbeit.
- **Plattformen:** Windows 64-bit + Linux 64-bit (Editor + Export-Templates). macOS später (Signierung/Notarisierung explizit nicht Teil dieser Aufgabe).
- **Sprache:** GDScript-Build ohne C#/.NET (Mono aus). UI-Strings im Godot-Code wie üblich auf Englisch mit `TTR(...)`. README für GO. auf Deutsch (mit kurzem englischem Abschnitt), technische Doku darf Englisch sein.
- **Compilieren:** Godot wird **nicht** vor jeder Kleinigkeit stundenlang lokal compiliert. Richte eine GitHub-Actions-Pipeline ein (§11) und lasse dort bauen. Lokale Builds nur, wenn die Umgebung dafür ausreicht; sonst SCons-Befehle dokumentieren und CI nutzen.
- **Git-Suite-Default bei „Neues Projekt“:** AN (Checkbox im Dialog vorbelegt, aber abwählbar) – passend zur bisherigen Godot-Vorauswahl „Version Control Metadata: Git“. Darunter eine zweite, eingerückte Checkbox **Git LFS aktivieren**, ebenfalls Default AN, die nur aktiv ist, solange die Git Suite angehakt ist, und die ohne Git-Repo deaktiviert ist. Es wird kein Remote voreingestellt oder erfunden; Host (GitHub/Codeberg/GitLab/Custom) wählt der User später im Remote-Assistenten (§6.1). LFS-Dateityp-Regeln werden auch ohne Remote eingerichtet, damit der allererste Commit schon korrekt Pointer statt Blobs enthält.
- **Code-Agentics-Default:** Dock vorhanden, aber der Agent verändert erst nach ausdrücklicher Nutzeranfrage etwas. Schreibende Tool-Aktionen zeigen standardmäßig eine Bestätigung (Auto-Approve nur als bewusst aktivierbarer Schalter **innerhalb** des Docks; das ist kein Provider-/Pfad-Setting und widerspricht §1.3 nicht).

### 3. Quellen, die du herunterladen/verlinken musst

#### 3.1 Kilo Code (vollständig clonen, als Referenz)
- Clone das komplette Kilo-Code-Repo (nicht nur einen Release-Zip):
  `git clone https://github.com/Kilo-Org/kilocode.git` in ein Verzeichnis **außerhalb** des committeten Godot-Source (z. B. `../kilocode-reference/` oder ein git-ignored `_external/kilocode/`) oder als gepinntes Git-Submodule. Committe **nicht** den kompletten Kilo-Source (ca. 200 MB Working Tree / ~700 MB mit History, plus `node_modules`) als kopierte Dateien in den Godot-Fork – das bläht den Fork auf und ist nicht updatefähig. Notiere die verwendete Kilo-Commit-ID in `docs/go/BASE_VERSION.md`.
- Kilo Code ist aktuell ein TypeScript-Monorepo (Basis: opencode) für **VS Code / JetBrains / CLI**. Es lässt sich **nicht** 1:1 in den C++-Godot-Editor einbetten. Deshalb: Studiere insbesondere `packages/kilo-gateway/` (dort `src/api/constants.ts`, `src/api/models.ts`, `src/api/url.ts`, `src/provider.ts`, `src/loader.ts`), `packages/core/` und `packages/kilo-vscode/` und **portiere das Verhalten nativ** (Chat-Dock, Model-Katalog, Free-Router, Agenten-Schleife) nach Godot. UI heißt bei uns **Code Agentics**; Kilo wird nur dort genannt, wo es technisch/rechtlich korrekt ist (Credits, Lizenz, kleiner Hinweis „Free models via Kilo Gateway“ im Dock-Footer ist OK).
- Kilo Code steht unter MIT (beachte auch die opencode-Herkunft in der LICENSE). Übernommene Konzepte/Code-Teile mit Copyright-/Lizenzhinweisen kennzeichnen, LICENSE-Datei der jeweiligen Komponente mitführen.
- **Nicht übernehmen:** Provider-/API-Key-Verwaltung, „Bring your own key“, MCP-Verwaltung, Workspace-/Multi-Root-Wechsel, Account-/Billing-UI, kostenpflichtige Modelle im Model-Picker.

#### 3.2 Bereits recherchierte Kilo-Gateway-Fakten (nicht neu recherchieren, nur verifizieren)
- Basis-URL: `https://ai-gateway.kilo.ai/api/v1` (OpenAI/OpenRouter-kompatibel).
- Modelle (ohne Auth): `GET https://ai-gateway.kilo.ai/api/v1/models` → JSON mit `data: [...]`, pro Modell u. a. `id`, `name`, `context_length`, `isFree`, `supported_parameters` (dort steht z. B. `tools`).
- Chat: `POST https://ai-gateway.kilo.ai/api/v1/chat/completions` (OpenAI-`messages`-Format; Streaming via SSE wenn `stream: true` – implementiere zuerst nicht-streamend stabil, Streaming erst wenn robust).
- Anonym-Free: Free-Modelle sind auch ohne Account erreichbar; Kilo nutzt für anonyme Requests den API-Key `"anonymous"` → Header `Authorization: Bearer anonymous`. Anonymes Rate-Limit: ca. 200 Requests/Stunde/IP – im Dock höflich anzeigen, wenn ein 429 kommt (mit `Retry-After`, falls vorhanden). Fehlertexte des Gateways nicht verschlucken.
- Free Router: `kilo-auto/free` (= Default für anonyme Nutzer; im Kilo-Source `DEFAULT_FREE_MODEL`). Für den Auto-Commit ebenfalls **nur** `kilo-auto/free` verwenden (nicht `kilo-auto/small`, das je nach Account-Guthaben auflösen könnte – wir haben prinzipiell keinen Account).
- **Erlaubte Modelle im Picker (Filter hart codieren):** `kilo-auto/free` **plus** alle live geladenen Modelle, die eindeutig gratis sind (`isFree == true` oder ID endet auf `:free` bzw. `pricing` ist nachweislich 0/0). Alles andere wird gar nicht angezeigt, auch wenn das Gateway es liefert. Wenn die Modellliste nicht lädt (offline), nur `kilo-auto/free` anbieten und Offline-Hinweis zeigen. Keine hartcodierte lange Liste veralteter Model-IDs.
- Datenschutz-Vermerk, der im Code-Agentics-Dock (klein, einmalig + im „?“/About des Docks) stehen soll: Gratis-Modelle können Prompts beim jeweiligen Anbieter loggen / zur Verbesserung nutzen (insb. NVIDIA-Free-Endpunkte sind Trial). Keine persönlichen oder geheimen Daten (Keys, Tokens) an den Agenten geben.

#### 3.3 Minecraft-Font (Lizenz!)
- Die originale Minecraft-Schrift (Mojang/Microsoft) **nicht** einbetten/verteilen. Verwende stattdessen eine frei lizenzierte Minecraft-Pixel-Font im selben Look, Standard: **Monocraft** (`https://github.com/IdreesInc/Monocraft`, OFL-1.1; TTF aus `dist/Monocraft-ttf/` oder dem Release `Monocraft-ttf.zip`). Font-Datei + OFL-Lizenztext ins Repo (z. B. `go_assets/fonts/Monocraft.ttf`, `go_assets/fonts/LICENSE-Monocraft.txt`).
- Die Font wird (a) ins Splash-Bild eingerendert und (b) für die GO.-Eigenen UI-Elemente (Splash-Zeile, ggf. Projekt-Manager-Fußzeile, Dock-Überschriften sparsam) als Godot-Font-Ressource nutzbar gemacht. Rest der Editor-UI bleibt in der normalen Godot-Schrift (Lesbarkeit!).
- Generator-Script (Python/Pillow: `go_assets/make_splash.py`) committen, so dass die Splash-PNGs reproduzierbar aus Logo + Font neu gebaut werden können.

### 4. Architektur-Vorgabe (wo im Godot-Code was hingehört)

Halte dich an Godot-Konventionen; schaue dir vor dem Coden vorhandene Beispiele an (`modules/*/SCsub`, `config.py`, `register_types.*`, `editor/plugins/*`, `editor/docks/*` bzw. die Ordnerstruktur der gewählten 4.x-Basis – in 4.7 liegt z. B. der Projekt-Manager unter `editor/project_manager/`).

- **Eigenes Engine-Modul** `modules/go_suite/` (Name darf an die konkrete Basis angepasst werden, Begründung dann in `docs/go/ARCHITECTURE.md`):
  - `config.py`: baut nur für Editor-Builds (`env.editor_build`); in Export-Templates ist die Suite wirkungslos/nicht enthalten.
  - Registriert unter `#ifdef TOOLS_ENABLED` **zwei Editor-Docks** als eingebaute Editor-Plugins: `GoGitDock` (Git Suite) und `GoAgenticsDock` (Code Agentics). Sie sind damit fester Teil des Editors (nicht von einem Projekt-Addon abhängig) und über Buttons im Seiten-/Dock-Bereich (rechts) ein-/ausblendbar, inkl. Icons (als SVG im Godot-Icons-Stil unter dem Modul).
  - Enthält: Git-Client (Prozess-Wrapper um die System-`git`), Kilo-Gateway-Client (HTTP über Godots `HTTPRequest`/TLS, JSON über `JSON`-Klasse), Agenten-Schleife + Godot-Tools (§9), Overlay-UI-Lock (§8), Lifecycle-Erinnerungen (§7), soweit sie aus dem Modul heraus lösbar sind.
- **Minimale, markierte Patches an Upstream-Dateien** (jeweils `// GO.`-Kommentar + Eintrag in `docs/go/UPSTREAM_PATCHES.md` mit Datei, Grund, Upstream-Merge-Hinweis):
  - `version.py` – Branding (§5).
  - `main/splash.png` ersetzen + **neu** `main/splash_editor.png` hinzufügen (§5).
  - `editor/project_manager/project_dialog.h/.cpp` (`ProjectDialog`) – Git-Suite-Auswahl bei „Neues Projekt“ + Projekt-Anlage (§7).
  - `editor/editor_node.cpp` – nur falls nötig für die Schließen-Erinnerung (bestehende Save-Confirmation-Logik um SCENE_QUIT nutzen, nicht duplizieren).
  - Icons/Logos: `main/app_icon.png`, `misc/logo/*`, Plattform-Export-Logos nur so weit ersetzen, dass Editor & Release einheitlich „GO.“ zeigen, ohne Export-Logik zu brechen.
- **Gebündelte Inhalte** (im Repo, werden ins Release kopiert und von der Engine ohne Netzwerk gefunden):
  - `go_bundled_plugins/` – vendored Plugins (§10) inkl. `plugins.json`-Manifest.
  - `go_project_template/` – das, was bei einem neuen GO.-Projekt zusätzlich zur normalen Godot-Anlage hineinkopiert wird (Boot-Splash, `.gitignore`-Ergänzungen falls nötig, `README`-Stub, aktivierte Plugins).
  - Lege fest und dokumentiere, wie der compilierte Editor diese Ordner findet (z. B. neben der Executable im Release: `go_bundled_plugins/`, Fallback Editor-Data-Dir, in Dev-Builds Quellbaum). Akzeptanz: Ein frisch erstelltes Projekt enthält die Plugins/Vorlage **ohne Internet**.
  - `docs/go/` – `BASE_VERSION.md`, `ARCHITECTURE.md`, `UPSTREAM_PATCHES.md`, `BUILDING.md`, `GIT_SUITE.md`, `CODE_AGENTICS.md`, `PLUGINS.md`, `ROADMAP.md`.
- **Projekt-Einstellungen:** `go/git_suite/enabled` (bool, aus der Projekt-Anlage), `go/git_suite/remind_on_open` (bool, Default an), `go/git_suite/remind_on_close` (bool, Default an), `go/git_suite/lfs/enabled` (bool, Default an wenn die Git Suite an ist), `go/git_suite/lfs/large_file_warning_mib` (int, Default 5). Der Git-Host (GitHub/Codeberg/GitLab/Custom) wird **nicht** als manuell zu pflegende Einstellung gespeichert, sondern bei jeder Remote-Änderung aus der `origin`-URL abgeleitet (zentrale Parser-Klasse, getestet, §6.1). Ebenso kommen in `modules/go_suite` eine zentrale `GoGitLfsManager`-Verantwortung (Erkennung/Installation/Init/Tracken/Status, §6.2) und eine zentrale Host-Erkennung – keine verstreuten Shell-Strings im UI-Code. Code Agentics braucht keine per-Projekt-Aktivierung (fester Editor-Bestandteil), darf aber nie ohne User-Aktion senden. Session-Verlauf von Code Agentics **nicht** ins Projekt-Git (in Editor-Config/User-Dir speichern und ggf. `.godot/` ist ohnehin ignored).

### 5. Branding & Splash (genau so)

- `version.py`: `name = "GO. Game Engine"`, `short_name = "go"` (beachte: ändert User-Daten-Verzeichnis + Binär-/Template-Namen – das ist für einen echten Fork gewollt und in `BUILDING.md` zu erklären), `website` auf das GO.-GitHub-Repo setzen, `docs` auf der Godot-Version lassen (Doku-Links müssen weiter funktionieren). Versionsnummer bleibt die Godot-Basis (4.7.2), damit Export-Template-Zuordnung nachvollziehbar bleibt; GO.-eigene Version (z. B. `GO. 0.1.0`) nur zusätzlich in About/README/Release-Namen ausweisen (z. B. `GO. Game Engine v4.7.2.go.0.1.0` in Dateinamen, Engine-Version-String nicht so verbiegen, dass die Template-Prüfung bricht – im Zweifel im Code prüfen und dokumentieren).
- **Editor-Splash:** In Godot 4.x fehlt `main/splash_editor.png` absichtlich im Repo (offizielles Artwork liegt nicht bei). `SConstruct` erkennt das und erzwingt `no_editor_splash`. Für GO.: `main/splash_editor.png` **neu erstellen** und mit `no_editor_splash=no` bauen, damit `main/main.cpp` (`boot_splash_editor_png`, generiert aus dieser Datei via `main/main_builders.py:make_splash_editor`) unseren Splash zeigt. Hintergrundfarbe des Editor-Splash kommt aus `boot_splash_editor_bg_color` – Bild darauf abstimmen oder bewusst (dokumentiert) anpassen. Das Bild wird zentriert und unskaliert angezeigt → vernünftige Größe (z. B. ca. 1152×648), nicht 4K.
- **Game-Splash:** `main/splash.png` (Godot-Original: 800×600) durch das GO.-Motiv in gleicher Größe ersetzen. Zusätzlich bekommen neue GO.-Projekte dasselbe Motiv als `application/boot_splash/image`.
- **Gestaltung beider Splash-Bilder:** Dunkler Hintergrund passend zum Godot-Editor, mittig das GO.-Logo / Schriftzug „GO.“ (eigenes, einfaches Logo – nicht das Godot-Robot-Logo verfremden), darunter klein „Game Engine“. **Unten rechts**, mit Abstand zum Rand, Pixel-Font (§3.3), exakt: `A Godot Game Engine Fork by SampleCraft`. Kein weiterer Text, keine Wasserzeichen-Orgie.
- Projekt-Manager + About-Dialog zeigen Produktname + GO.-Version; ein dezenter Footer im Projekt-Manager mit demselben SampleCraft-Text (Pixel-Font, klein) ist zusätzlich erlaubt, ersetzt aber **nicht** den Splash-Text.

### 6. Suite 1: GO. Git Suite (Dock)

Das Git-Dock bedient normales Git über die installierte System-`git` (Prozesse asynchron/nicht UI-blockierend ausführen; bei fehlender `git` einen klaren Fehlerdialog mit Installationshinweis zeigen, Editor darf nicht abstürzen). Falls das mitgelieferte Godot Git Plugin (EditorVCSInterface/libgit2, §10) im Projekt aktiv ist, darfst du es ergänzend nutzen – die Kernfunktionen (Status/Commit/Push/Pull/Automatic Commit) müssen bereits mit System-Git funktionieren. Alle Git-Befehle laufen mit `-C <Projektordner>` und niemals in einem anderen Verzeichnis.

Mindestumfang des Docks:
- Kopfzeile: aktueller Branch, Remote (z. B. `origin`) **mit erkanntem Host-Chip** (§6.1: GitHub / Codeberg / GitLab / Custom / nur lokal), Zustand (clean / X geändert / ahead/behind nach Fetch), **Git-LFS-Status-Chip** (§6.2: nicht installiert / nicht initialisiert / bereit, N Dateien, Gesamtgröße), Button **Aktualisieren (Fetch + Status)**.
- Änderungsliste (`git status --porcelain`): staged/unstaged/untracked erkennbar, Datei anklicken → Diff-Anzeige (Text-Diff, gekürzt wenn riesig) und „Im Editor/Dateisystem zeigen“, wo möglich. LFS-verwaltete Dateien sind mit einem `LFS`-Badge gekennzeichnet; große, noch **nicht** LFS-getrackte Binärdateien (§6.2) bekommen eine Warn-Markierung mit Ein-Klick-Aktion „Mit Git LFS tracken“.
- Stage/Unstage (einzeln + „Alle stagen“), Verwerfen von Änderungen **nur** mit Sicherheits-Rückfrage. Beim Stagen gilt die Reihenfolge aus §6.2: Die passende `.gitattributes`-Regel muss im Working Tree stehen, **bevor** eine LFS-Datei ins Staging geht, sonst landet der Blob statt des Pointers in Git.
- Commit-Bereich: Message-Feld (Subject + optionaler Body), Button **Commit**.
- Sync: Buttons **Pull** (danach automatisch LFS-Objekte materialisieren, `git lfs pull` bzw. die installierten Filter wirken lassen; kein Auto-Merge-Zauber: bei Konflikten sauber melden und Git-Zustand anzeigen, nichts still überschreiben) und **Push** (der von `git lfs install` gesetzte Pre-Push-Hook lädt die LFS-Objekte mit hoch; fehlt er, vor dem Push per `git lfs install --local` reparieren und das im Status zeigen). Zusätzlich ein Button **Im Browser öffnen**, der aus der Remote-URL die Repo-Webseite des erkannten Hosts öffnet (Codeberg: `https://codeberg.org/<owner>/<repo>`).
- Verlauf: letzte Commits (`git log`, z. B. 30 Einträge: Hash kurz, Subject, Autor, Datum).
- Repo-Aktionen: **Git initialisieren** (falls Projekt noch keins ist, aber Git Suite aktiv werden soll; `git init -b main`), „Remote setzen/ändern“ (URL-Feld für `origin`), Link/Hinweis zu den Git-Credentials des Systems (Passwörter/Tokens speichert GO. **nie selbst** – Auth macht Git / Credential Helper / SSH des Users).
- Großer, hervorgehobener Button **`Automatic Commit`** (§8).
- Leere/Fehlerzustände sind gestaltet: kein Repo, kein Remote, kein Upstream, detached HEAD, Merge-Konflikt, `user.name`/`user.email` fehlt, Push abgelehnt (non-fast-forward → Hinweis „erst Pull“), Git fehlt, Git LFS fehlt/nicht initialisiert, LFS-Pointer nicht heruntergeladen, LFS-Quota/Auth-Fehler beim Hochladen. Kein `push --force`, kein `reset --hard`, kein Amend fremder/gepushter Commits – nirgendwo im Feature.

#### 6.1 Git-Hosts: Codeberg, GitHub, GitLab und generisches Git

- **Prinzip:** Normales Git über die `origin`-URL – keine host-spezifische Sonder-Git-Engine. Alle Kernfunktionen (Fetch/Pull/Push/Automatic Commit/LFS) funktionieren mit jedem Host. Der Host wird nur für Anzeige, Browser-Links, Remote-Erzeugung aus Owner/Repo und host-spezifische Warnhinweise erkannt.
- **Host-Erkennung (zentraler, unit-getesteter Parser):** Eingaben robust parsen und auf `host`, `owner`, `repo`, `protocol` (`https`/`ssh`) und Web-URL normalisieren. Diese Formen müssen alle aufs selbe Repo zeigen: `https://codeberg.org/knut/foobar`, `https://codeberg.org/knut/foobar.git`, `git@codeberg.org:knut/foobar.git`, `ssh://git@codeberg.org/knut/foobar.git` – analog für `github.com` und `gitlab.com`. Alles Unbekannte (selbst-gehostetes Forgejo/Gitea/GitLab, lokale Pfade, `file://`) wird als `Custom`/`Local` klassifiziert und funktioniert trotzdem; Web-URL dann aus der HTTPS-Remote-URL ableiten, wenn möglich, sonst Button ausblenden. Testfälle für den Parser sind Pflicht (mit/ohne `.git`, Trailing-Slash, Port in `ssh://`, Groß-/Kleinschreibung, User statt Organisation).
- **Remote-Assistent (Modal aus dem Git-Dock):** Host-Chips **GitHub / Codeberg / GitLab / Benutzerdef. URL**. Felder: Owner (Benutzer oder Organisation), Repository-Name, Protokoll (HTTPS/SSH). Live-Vorschau der erzeugten URL, Codeberg exakt als `https://codeberg.org/<owner>/<repo>.git` bzw. `git@codeberg.org:<owner>/<repo>.git` (GitHub/GitLab analog mit ihren Hosts; bei Custom ein freies URL-Feld). Buttons:
  - **Als `origin` setzen** (`remote add` oder – mit Rückfrage – `remote set-url`), danach Verbindungstest (`git ls-remote origin`, zeitlich begrenzt, Ergebnis ehrlich anzeigen; ein noch leeres Remote ist ein Erfolg).
  - **Neues Repo beim Host anlegen** öffnet die Erstell-Seite im Browser und lässt den User dort (mit seinem normalen Login) das Repo anlegen: Codeberg `https://codeberg.org/repo/create`, GitHub `https://github.com/new`, GitLab `https://gitlab.com/projects/new`. Danach hilft der Assistent beim Übernehmen der URL. In v1 **keine** token-basierte Repo-Erstellung per API einbauen und **keine** Zugangsdaten in GO. abfragen oder speichern (die Forgejo-API von Codeberg, `POST /api/v1/user/repos`, darf in `docs/go/ROADMAP.md` als späterer, token-sparsamer Ausbau notiert werden).
  - Hilfetexte je Host: SSH-Key einrichten (Link auf die Host-Doku, Codeberg: `https://docs.codeberg.org/security/ssh-key/`), bei HTTPS der normale Credential Helper des System-Git. Ein Button **Repo im Browser öffnen** erscheint, sobald ein Web-Ziel ableitbar ist.
- **Codeberg-Besonderheiten ehrlich abbilden (nicht GitHub annehmen):** Codeberg basiert auf Forgejo und unterstützt Git LFS offiziell – keine LFS-Features hinter einer GitHub-Annahme verstecken. Speicher-Richtwerte von Codeberg (Stand der Recherche: ca. **750 MiB Git-Speicher** und **1,5 GiB für LFS/Pakete/Anhänge zusammen**, Überschreitung eigentlich vorher anzufragen) als **konfigurierbare Warnwerte im Code + Doku-Link** abbilden, nicht als fest verdrahtetes Naturgesetz: Zeigt das Git-Dock bei Codeberg-Host eine genutzte LFS-Gesamtgröße, die sich der Schwelle nähert, eine freundliche Warnung mit Link auf die Codeberg-Doku (`https://docs.codeberg.org/git/using-lfs/`), statt Pushes von sich aus zu verbieten. Analoge Schwellen für GitHub (Datei-Hard-Limit ohne LFS) im Large-File-Hinweis (§6.2) berücksichtigen; bei Custom/Local keine erfundenen Quoten anzeigen.

#### 6.2 Git LFS: automatische Installation, Initialisierung und große Dateien

Ziel: Ein User, der ein GO.-Projekt mit Git Suite anlegt, muss nie die Konsole öffnen, damit große Assets (Texturen, Audio, 3D, Video, .blend/.psd usw.) als LFS-Pointer statt als Git-Blobs landen.

**A. Installation („automatisch“, aber OS-ehrlich):**
- Beim ersten Bedarf (Projekt-Anlage mit LFS-Häkchen, §7, oder „Git LFS einrichten“ im Dock) prüfen: `git lfs version`. Installiert → weiter mit B. Nicht installiert → **Ein-Klick-Installationsdialog** mit Plattform-Autoerkennung und klarem Text, was passieren wird (Download-Quelle, benötigte Rechte) + Fortschrittsbalken:
  - **Windows:** Git for Windows liefert Git LFS normalerweise bereits mit – der fehlende Fall bedeutet meist eine Portable/Minimal-Installation. Reihenfolge: erst `winget install --id GitHub.GitLFS -e` anbieten, wenn `winget` existiert (UAC zeigt Windows selbst; der Editor wartet nicht-blockierend auf das Prozessende). Fallback: offiziellen Installer von `https://git-lfs.com/` (Git-LFS-Releases) herunterladen, SHA256 gegen die Release-Checksummen prüfen und für den User starten. Keinen inoffiziellen Mirror, keine stille Systeminstallation ohne die übliche UAC-Bestätigung.
  - **Linux:** Paketmanager erkennen (`apt`, `dnf`, `pacman`, `zypper`) und den passenden Befehl (Paket `git-lfs`) als **Ein-Klick-Aktion über `pkexec`** (grafische Admin-Abfrage) ausführen; geht das nicht, den exakten Terminal-Befehl zum Kopieren anzeigen. Zusätzlich als nicht-privilegierte Alternative anbieten, das offizielle Git-LFS-Binary in ein User-Verzeichnis (z. B. `~/.local/bin`) zu legen und den fehlenden `PATH`-Eintrag zu erklären. Keine Passwörter im Editor abfragen.
  - Nach jeder Installationsart Erfolg per `git lfs version` verifizieren. Scheitert sie, bleibt das Projekt voll nutzbar; `go/git_suite/lfs/enabled` steht auf „Einrichtung ausstehend“, das Git-Dock zeigt ein dauerhaftes, nicht-nerviges Setup-Banner mit „Erneut versuchen“-Button. Nichts davon darf den Editor blockieren oder abstürzen lassen.
- Installiert heißt nicht aktiviert: Zusätzlich `git lfs install` (einmalig global, Hooks/Filter im User-Git) und `git lfs install --local` (pro Projekt) ausführen und im Dock-Status unterscheiden (`nicht installiert` / `installiert, im Projekt inaktiv` / `bereit`).

**B. Initialisierung im Projekt (Reihenfolge ist entscheidend):**
1. Repo muss existieren (`git init -b main`), dann `git lfs install --local`.
2. **Danach** die `.gitattributes`-Regeln schreiben lassen – bevorzugt über `git lfs track "<pattern>"`, damit das Tool selbst das Format (`filter=lfs diff=lfs merge=lfs -text`) erzeugt – und zwar **bevor** große Dateien das erste Mal gestaged werden und auch bevor gebündelte Plugin-Dateien, die später mitcommitted werden, ins Staging gelangen. Godots eigene Zeile `* text=auto eol=lf` (aus `create_vcs_metadata_files`) **erhalten**; die LFS-Muster mit `-text` gewinnen für ihre Dateitypen. Einen mit `# GO. Git LFS (automatisch verwaltet)` markierten Block verwenden und beim Aktualisieren nur diesen Block/fehlende Muster ergänzen – nutzereigene `.gitattributes`-Zeilen nie still löschen oder umordnen.
3. **Standard-Muster** (Godot-relevante Binärformate; der neue Agent darf sie um weitere belegte Formate erweitern, Liste in `docs/go/GIT_SUITE.md` pflegen):
   - Bilder: `*.png *.jpg *.jpeg *.tga *.tif *.tiff *.webp *.exr *.hdr *.bmp *.dds *.ktx *.ktx2`
   - Arbeitsdateien DCC: `*.psd *.kra *.xcf *.aseprite *.blend *.sbsar`
   - 3D: `*.glb *.fbx *.dae *.bin` (`.bin` = externe glTF-Buffer)
   - Godot-Binärressourcen: `*.res *.scn`
   - Audio: `*.wav *.ogg *.mp3 *.flac *.aif *.aiff`
   - Video: `*.mp4 *.webm *.mov *.ogv`
   - Fonts: `*.ttf *.otf *.woff *.woff2`
   - Archive (nur wenn doch versioniert): `*.zip`
   - **Bewusst NICHT** in LFS: Textformate, deren Diffs/Merges wertvoll sind – `*.tscn *.tres *.gltf *.gd *.import *.uid *.svg *.json *.cfg *.godot` bleiben normale Git-Dateien, auch wenn sie groß werden (dafür gibt es nur den Warnhinweis aus C). Export-Ergebnisse (`.pck`, Exe/APK-Ordner) gehören in die `.gitignore`, nicht in LFS; die GO.-`.gitignore`-Ergänzung listet übliche Export-Artefakte (auf Godots `.godot/`-Eintrag aufbauen, nichts doppeln).
4. Die so entstandene `.gitattributes` muss mit in den ersten Commit; das Git-Dock zeigt bei neuem Projekt so lange einen Hinweis „`.gitattributes` noch nicht committed“, bis sie in `HEAD` liegt – sie ist die Datei, ohne die Teammitglieder kein LFS-Verhalten bekommen.
5. Projekte ohne LFS-Häkchen oder mit `lfs/enabled=false`: keine Regeln anfassen, keine Installationsdialoge, nur der Setup-Button im Dock bleibt sichtbar.

**C. Automatische Behandlung großer Dateien im Alltag:**
- Zentrale Prüfung (wird vom Git-Dock-Refresh, vom manuellen Stagen und der Auto-Commit-Vorprüfung §8 genutzt): Alle geänderten/neuen Dateien aus dem Status mit Dateigrößen anreichern. Regeln:
  1. Binär-Asset (§B-Liste oder nicht als Text erkennbar) und nicht von einer LFS-Regel gedeckt → Ein-Klick-Fix „Mit Git LFS tracken“ (nach Endung tracken, bzw. bei Einzelstück ausnahmsweise pfadgenau; danach Datei neu stagen, so dass der Pointer im Index landet).
  2. Beliebige Datei über `large_file_warning_mib` (Default 5 MiB) → Warnzeile im Dock mit Dateiname + Größe; ist sie Text (s. o.), nur warnen und Alternativen erklären („aufteilen / `.gitignore` / trotzdem committen“), nicht eigenmächtig in LFS zwingen.
  3. Datei, die ohne LFS absehbar am Host scheitert (Richtwert >50 MiB Warnstufe rot; GitHub hart 100 MiB pro Datei ohne LFS; für Codeberg/anderer Host die §6.1-Warnwerte) → **manueller Commit und Automatic Commit warnen bzw. blockieren**, bis sie LFS-getrackt (oder entfernt/ignored) ist. „Trotzdem normal committen“ darf es nur als bewusste, beschriftete Ausnahme unterhalb der harten Host-Grenze geben.
- Nach dem Stagen **verifizieren**, dass der Index für LFS-Dateien wirklich Pointer enthält (z. B. `git lfs ls-files`/`git lfs status` bzw. gestagten Blob-Anfang auf `version https://git-lfs.github.com/spec/v1` prüfen). Zeigt die Verifikation einen Blob (meist: Datei war vor der Regel schon gestaged), den Index für diese Dateien reparieren (erneut `git add`), statt den Fehler in einen Commit zu lassen.
- **Download-Seite:** Pull/Checkout/Öffnen muss fehlende Inhalte erkennen: Erkannte Pointer-Dateien im Working Tree (Dateianfang ist der LFS-Pointer / `git lfs ls-files` zeigt nicht materialisierte Dateien) führen zu einem Banner + Button **LFS-Dateien herunterladen** (`git lfs pull`), inkl. Fortschritt. Nach einem erfolgreichen `git pull` im Dock denselben Materialisierungs-Schritt automatisch anstoßen (installierte Smudge-Filter übernehmen den Normalfall; der explizite `git lfs pull` ist die Reparatur), und Godot danach zum Rescan der betroffenen Ressourcen veranlassen – sonst zeigt der Editor scheinbar kaputte 130-Byte-„Texturen“.
- **Bestehende Historie:** Dateien, die vor der LFS-Einrichtung bereits als Blobs committed wurden, bleiben unverändert. Es gibt nur einen freiwilligen Migrations-Assistenten (Button „Historie auf LFS migrieren …“ darf existieren), der `git lfs migrate import` mit ausdrücklicher, mehrfacher Warnung erklärt (schreibt **alle** Commit-Hashes um, Team muss neu clonen, Force-Push nötig) und eine Backup-Branch anlegt. Das ist explizit **kein** Teil von „automatisch“, nie Teil des Automatic Commit und im Zweifel ein Roadmap-Punkt statt überhastet einzubauen.
- **Quota-Anzeige:** Im Dock (ausklappbarer LFS-Bereich): Anzahl + Gesamtgröße der getrackten LFS-Dateien im HEAD (`git lfs ls-files --size`, sonst aus Dateigrößen summieren), größte 5 Dateien, noch nicht heruntergeladene/nicht hochgeladene Objekte, wenn praktisch ermittelbar (z. B. `git lfs push --dry-run`) – und bei Codeberg/GitHub der §6.1-Warnhinweis, sobald die konfigurierten Schwellen in Sicht kommen.

### 7. Projekt-Lifecycle (Anlegen / Öffnen / Schließen)

**Neues Projekt (`ProjectDialog`, MODE_NEW):**
- Es gibt dort schon „Version Control Metadata“ (`vcs_metadata_selection`, None/Git, Default Git) mit `EditorVCSInterface::create_vcs_metadata_files(...)` (legt `.gitignore`/`.gitattributes` für Godot an, macht aber **kein** `git init`). Baue **darauf auf**, dupliziere nichts.
- Füge darunter eine GO.-Checkbox hinzu (Englisch, TTR): **`Enable GO. Git Suite for this project (initialize Git repository)`**, Default AN, Tooltip erklärt: Initialisiert ein Git-Repo, aktiviert Git Suite + Erinnerungen, kopiert gebündelte Plugins. Wenn „Version Control Metadata: None“ gewählt ist, ist die Checkbox deaktiviert (mit verständlichem Grund im Tooltip).
- Bei „Create“ mit aktiver Suite (nur MODE_NEW, nicht bei Import/Install ungefragt):
  1. Normale Godot-Projektanlage (project.godot, icon, VCS-Metadaten) wie upstream.
  2. `go/git_suite/enabled = true` (+ Reminder-Defaults) in die `project.godot`.
  3. Gebündelte Plugins (§10) + GO.-Projektvorlage hineinkopieren, Kern-Plugins unter `[editor_plugins] enabled=...` eintragen, Boot-Splash (§5) setzen.
  4. `git init -b main` im Projektordner (nur wenn noch kein `.git` existiert). **Kein** ungefragter erster Commit und kein erfundener Remote; schlägt Git fehl (nicht installiert etc.), Projekt trotzdem anlegen und dem User eine klare Meldung + den Init-Button im Git-Dock zeigen.
- Ohne Häkchen: Projekt exakt wie bei Godot (abgesehen vom GO.-Boot-Splash), kein `.git`, keine Git-Erinnerungen (`go/git_suite/enabled = false` oder Key weglassen und Default=false an dieser Stelle dokumentieren).

**Projekt öffnen (Reminder):**
- Wenn `go/git_suite/enabled` und `.git` existiert und der Editor fertig geladen hat (Splash nicht blockieren): asynchron `git fetch` (falls Remote existiert) + Status prüfen. Dann **ein** freundlicher Dialog (mit „Nicht mehr für dieses Projekt anzeigen“-Häkchen = `remind_on_open= false`), je nach Lage:
  - „Auf dem Remote gibt es N neuere Commits – Pull empfohlen“ (Button: **Pull** / Später).
  - „Du hast X uncommittete Änderungen / Y ungepushte Commits“ (Buttons: **Git Suite öffnen** / Später).
- Sauber & aktuell → **kein** Dialog (keine Belästigung). Fetch-Fehler offline → still übergehen (nur Log), kein Fehler-Popup beim Start.
- Projekt mit `.git`, aber ohne aktivierte Suite → einmalig anbieten, die Suite zu aktivieren (kein Auto-Aktivieren, kein Auto-Init).

**Projekt/Editor schließen (Reminder):**
- In die bestehende Unsaved-Changes-Logik von `EditorNode` (Save-Confirmation bei Quit / Rückkehr in den Projekt-Manager) einklinken: Wenn Git Suite an + (uncommittete Änderungen oder ungepushte Commits oder – soweit nach §6.2 C ermittelbar – noch nicht hochgeladene LFS-Objekte), vor dem endgültigen Schließen ein Dialog: „Noch nicht alles gesichert: X uncommittete Änderungen, Y ungepushte Commits (, Z LFS-Datei(en) nicht hochgeladen)“. Buttons: **Automatic Commit** (führt §8 aus, danach schließen fortsetzen, wenn erfolgreich / User bestätigt), **Trotzdem schließen**, **Abbrechen**. „Nicht mehr anzeigen“-Häkchen (`remind_on_close`). Clean + alles (inkl. LFS) gepusht → kein Dialog.
- Niemals das Schließen endlos blockieren: Läuft bereits ein Git-/Auto-Commit-Prozess, zuerst dessen Ende/Fehler sauber behandeln (§8-Lock), keine zweite Git-Operation parallel.

### 8. „Automatic Commit“ (Herzstück, exakt dieser Ablauf)

1. **Vorprüfung (UI noch offen):** Ist es ein Git-Repo? Gibt es überhaupt Änderungen? Merge-Konflikt? `user.name/email` gesetzt? Wenn etwas Grundsätzliches fehlt → normale Fehlermeldung, ** kein** Lock, kein Commit. Falls kein Remote/Upstream: weiter (es wird dann nur committed und danach klar gemeldet „Kein Remote – nicht gepusht“).
2. **UI locken:** Full-Editor-Overlay (halbtransparent, frisst alle Eingaben, Editor dahinter sichtbar ausgegraut), mittig: Titel „Automatic Commit läuft …“, **Progress Bar** (Etappen + indeterminierte Animation, wo keine echte Prozentzahl möglich ist) und eine Statuszeile, die die aktuelle Etappe anzeigt. Fenster-Schließen/Editor-Quit während des Locks abfangen oder verhindern. Etappen:
   1. „Änderungen einlesen …“ (Status, `diff --stat`, gekürzter `diff`)
   2. „Commit-Message wird über Kilo (Free Router) erzeugt …“
   3. „Dateien werden gestaged …“ (falls die Vorprüfung `.gitattributes`/LFS gerade eingerichtet hat, die betroffenen großen Dateien gezielt (erneut) `git add`en und nach §6.2 C verifizieren, dass im Index Pointer liegen; danach `git add -A` – `.gitignore` beachten; `.godot/`-Cache gehört da ohnehin nicht rein)
   4. „Commit wird erstellt …“
   5. „Wird gepusht … (inkl. Git-LFS-Objekte, falls vorhanden)“ (übersprungen, wenn kein Remote; mit `-u origin <branch>`, wenn noch kein Upstream existiert und ein `origin` da ist; der LFS-Pre-Push-Hook lädt die Objekte während des Pushs – eine eigene LFS-Fortschrittsmeldung aus der Prozessausgabe anzeigen, wo möglich)
   6. „Fertig.“ → Overlay schließt sich, danach kurze Erfolgsmeldung mit Commit-Hash + verwendeter Message (+ „N LFS-Datei(en), MM MiB übertragen“, wenn LFS beteiligt war).
3. **Commit-Message per Kilo (eigene, deterministische Implementierung – der Chat-Agent soll hier nicht frei Kommandos erfinden):** Ein einzelner `chat/completions`-Request an `kilo-auto/free` mit: Status, Diff-Stat und einem **hart gekürzten** Text-Diff (z. B. max. ~12.000 Zeichen, Binär-/Lockdateien nur namentlich, keine Dateien, die nach Secrets aussehen – `.env`, `*key*`, `*token*` – inhaltlich mitsenden). Interner System-Prompt sinngemäß: „You write git commit messages. Output ONLY a Conventional-Commit subject (max. 72 chars, no trailing period, no markdown, no quotes) and, if useful, a short body with bullet points. Summarize WHAT changed and WHY.“ Die Antwort wird sanitisiert (Code-Fences/Quotes entfernen, erste Zeile = Subject, Längen begrenzen). Netzwerk-/Kilo-Fehler → Fallback-Message (z. B. `chore: update project files`) und Hinweis im Ergebnis – der Commit darf daran nicht scheitern.
4. **Fehler:** Jede Etappe kann fehlschlagen (Push-Rejektion, Netzwerk, Hook schlägt fehl …) → Overlay schließen, die UI wieder freigeben, verständliche Meldung mit der echten Git-Ausgabe (einklappbar/Detailbereich), Zwischenstand ehrlich benennen („Commit erstellt, Push fehlgeschlagen – bitte Pull ausführen“). **Kein** Force-Push, keine automatische Konfliktlösung, keine Endlosschleife. Abbruch-Button im Overlay nur, wo es sicher ist (vor dem Push); während `git add/commit/push` läuft: kein hartes Killen, das das Repo zerschießen könnte.
5. Nach dem Vorgang: Git-Dock refresh, wenn der Auslöser der Schließen-Dialog war (§7), den Schließen-Flow erst danach fortsetzen.

### 9. Suite 2: Code Agentics (umgebrandetes Kilo Code)

**Dock-UI (Kilo-artig, aber Godot-nativ):**
- Header: Titel **Code Agentics**, Model-Picker (nur Free-Modelle, §3.2, Default `kilo-auto/free`), kleiner Live-Hinweis, wenn die Modellliste aus dem Gateway geladen wurde vs. Offline-Fallback.
- Darunter eine **nicht editierbare** Zeile mit Schloss-Symbol: `Workspace: <absoluter Projektpfad> (res://) – locked`.
- Chat-Verlauf (User / Assistant / Tool-Aktionen als kompakte Einträge: welcher Tool-Name, welche Datei/Szene, Erfolg/Fehler), Eingabefeld (Enter = senden, Shift+Enter = neue Zeile), **Send**- und **Abbrechen**-Button, Fortschrittsanzeige während der Agent arbeitet (dieser Abort stoppt die Agenten-Schleife kontrolliert nach dem aktuellen Schritt).
- Buttons/Leiste: „Neue Konversation“, Verlauf (aus Editor-Speicher, §4), Auto-Approve-Schalter (§2), Hilfe/About (Datenschutz-Hinweis §3.2, Credits Kilo/opencode, Liste der Godot-Tools).
- **Exakt nicht vorhanden:** Provider-/Model-Settings über den Free-Picker hinaus, API-Key-/Base-URL-Felder, Kilo-Login, MCP, Workspace-Auswahl, „Terminal öffnen“. Prüfe die fertige UI wie ein Reviewer: Wer diese Dinge sucht, darf sie nicht finden.
- Kontext, den der Agent **automatisch** bekommt: Projektname, Godot-/GO.-Version, aktuell geöffnete Szene + Szenenbaum (zusammengefasst), ausgewählte Nodes, Liste der Projektdateien (gekürzt, kein `.git/`/`.godot/`-Inhalt), verfügbare Tools (§9). System-Prompt anweist u. a.: Nur über die Tools handeln, nichts behaupten, was kein Tool gemacht hat, alle Pfade als `res://`, kleine Schritte, bei Fehlern Tool-Ergebnisse ernst nehmen, GDScript-Godot-4-Syntax, keine Secrets anfordern.

**Agenten-Schleife:**
- HTTP an das Kilo Gateway (§3.2), `Authorization: Bearer anonymous`, sinnvolles Timeout, `X-KILOCODE-EDITORNAME` bzw. User-Agent darf `GO. Game Engine (Code Agentics)` lauten.
- Bevorzuge das native OpenAI-kompatible Tool-Calling, wenn das gewählte Free-Modell `tools` in `supported_parameters` führt. Fallback für Modelle ohne Tool-Calling: ein klar definiertes, geparstes JSON-Tool-Format in der Antwort (im Code `CODE_AGENTICS.md` dokumentieren). Niemals Free-Text „ausführen“.
- Max. Schritte pro User-Auftrag (z. B. 20 Tool-Runden), danach Zwischenstand zusammenfassen und stoppen. Tool-Ergebnisse werden gekürzt an das Modell zurückgegeben (Datei-Auszüge/Diffs begrenzen). Lese-Tools automatisch; schreibende Tools nur nach Bestätigungsdialog (Diff/Vorschau zeigen, „Anwenden/Ablehnen“), außer Auto-Approve ist an. **Kein** generisches Shell-Tool in v1 (kein `run_command` mit freiem Text).
- Undo/Redo: Szenen-/Ressourcen-Änderungen über den Editor-UndoManager (`EditorInterface`/UndoRedo der Basis), wo die API das hergibt; Datei-Änderungen danach im Godot-Dateisystem scannen/importieren, offene Szenen ggf. neu laden, ohne ungespeicherte User-Arbeit still zu verwerfen (Konfliktfall melden).
- Fehler aus Godot (Parse-Fehler eines erstellten Scripts, ungültige Property, Szene lässt sich nicht speichern) werden als Tool-Ergebnis an das Modell zurückgegeben, nicht als Editor-Absturz. Alle Eingaben des Modells werden validiert (Typen, Node-Klassen existieren, Property existiert und Typ passt, Ressourcen-Pfade zulässig).

**Pflicht-Tools des Agenten (Whitelist, alles nur innerhalb `res://`):**
- Dateien: `list_files` (optional Glob), `read_file` (mit Zeilenlimit), `write_file`, `edit_file` (exaktes Ersetzen eines eindeutigen Blocks), `create_folder`, `delete_file` (nur mit Bestätigung, nie `.git/`), `search_text` (grep-artig über Projektdateien).
- Scripte: `create_script` (GDScript, optional Template/extended Klasse), `attach_script` (an Node in offener/angegebener Szene), `get_script_errors` bzw. ein `validate_script`, wenn die Basis-API das praktikabel anbietet (sonst: Hinweise, wie geprüft wurde, ehrlich dokumentieren).
- Szenen/Nodes: `create_scene` (mit Root-Typ), `open_scene`, `get_current_scene_tree`, `get_scene_tree` (für eine `res://`-Szene), `add_node` (Typ, Name, Elternpfad), `remove_node`, `rename_node`, `move_node` (Reihenfolge/Eltern), `set_node_property` / `get_node_property`, `save_current_scene`, `save_scene_as`, `instantiate_scene` (Szene als Kind einfügen).
- Ressourcen/Projekt: `create_resource` (nur gängige, sicher erzeugbare Typen, z. B. via `ResourceSaver`), `get_project_setting`, `set_project_setting` (Whitelist sinnvoll begrenzen, z. B. Boot-Splash/Main-Scene/Input ist Thema – `run/main_scene` setzen gehört dazu), `set_main_scene`, `scan_filesystem` (Import/Rescan auslösen), `get_editor_selection`, `select_node`.
- Ausführen: `run_project`, `run_current_scene`, `stop_project`, `get_editor_log` (letzte Editor-/Debugger-Meldungen, soweit API-zugänglich; sonst ehrlich einschränken).
- Git (nur lesend + der eine definierte Schreibpfad): `git_status`, `auto_commit` (löst exakt den §8-Ablauf inkl. UI-Lock aus – kein separates, lockfreies Commit-Tool für das Modell).
Jedes Tool: Name, Parameter-Schema, Rückgabe und Fehlerfälle in `docs/go/CODE_AGENTICS.md` dokumentieren. Pfad-Prüfung zentral implementieren und testen (`res://`-Normalisierung, `..` ablehnen, absolute Pfade ablehnen, `.git/` schreibgeschützt, `.godot/` nicht als Quell-Datei behandeln).

**Smoke-Test, der manuell belegt werden muss (Screenshot/Log in PR-Beschreibung):** Prompt an Code Agentics: „Erstelle eine 2D-Szene `player.tscn` mit einem `CharacterBody2D` als Root, einem `Sprite2D`- und einem `CollisionShape2D`-Kind, erstelle `player.gd` mit einfacher Links/Rechts-Bewegung, hänge das Script an, speichere die Szene und setze sie als Main-Scene.“ → Alle Schritte laufen über Tools, Szene öffnet sich danach fehlerfrei im Editor, Undo funktioniert für die Node-Schritte.

### 10. Fest integrierte Plugins

Prinzip: Plugincode liegt im Fork unter `go_bundled_plugins/<plugin>/` (vendored, mit Upstream-LICENSE unverändert) und wird bei der Projekt-Anlage (§7) nach `res://addons/<...>/` kopiert. **Reine GDScript-Addons bleiben Addons** (das ist ihre upstream vorgesehene Form) – „fest in GO. drin“ heißt: im Repo & Release enthalten, ohne Download, von GO. installiert/aktiviert. **GDExtension-Plugins** (Terrain3D, LimboAI, Git Plugin, Debug Draw 3D) werden nicht in den Engine-Core „hineinkopiert“, sondern je Plattform gebaut bzw. als upstream Release-Binaries mit Prüfsummen fest ins GO.-Release gebündelt (in CI, §11); sie dürfen deshalb im Git nur so groß werden, wie es upstream selbst ist – keine wahllosen 500-MB-Binär-Commits (im Zweifel: Source + CI-Build + `plugins.json`-Pin). Jedes Plugin bekommt einen Eintrag in `go_bundled_plugins/plugins.json`: `name`, `upstream_url`, `license`, `version/tag/commit` (gepinnt, kein „latest“ zur Runtime), `godot_min`, `addon_path`, `type` (`gdscript`/`gdextension`), `default_enabled` (`true/false`), `category`, `notes` (Konflikte/Plattformen). Vor dem Vendoren **prüfen**: Lizenz ist MIT/kompatibel, Version läuft mit der gewählten Godot-Basis, Upstream-README-Installationspfad stimmt. Eine GO.-Seite „GO. → Bundled Plugins“ (Menü oder Bereich im Git-/Projekt-Kontext) listet alle Plugins, zeigt aktiviert/aus und kann sie **offline** in ein bereits offenes Projekt kopieren/aktivieren/deaktivieren (Deaktivieren = Godot-Plugin aus; Dateien löschen nur auf ausdrücklichen Wunsch).

Kernliste (alle MIT, Upstream-URL verifizieren; exakte Tags auf die Godot-Basis pinnen und ein leeres GO.-Projekt mit den `default_enabled`-Plugins fehlerfrei öffnen – schlägt etwas fehl, `default_enabled=false` setzen und in `PLUGINS.md` begründen, statt einen kaputten Default zu lassen):
- **Godot Git Plugin** – `godotengine/godot-git-plugin` (GDExtension/EditorVCSInterface; harmoniert mit §6).
- **Dialogic 2** – `dialogic-godot/dialogic` (Dialoge; Kern, `default_enabled=true` nach Test).
- **Dialogue Manager** – `nathanhoad/godot_dialogue_manager` (Alternative zu Dialogic: gebündelt, aber **nicht gleichzeitig** mit Dialogic default-aktiv; im Dialog/Menü gegenseitig ausschließend anbieten).
- **Phantom Camera** – `ramokz/phantom-camera` (2D/3D-Kamera; Kern).
- **LimboAI** – `limbonaut/limboai` (Behavior Trees + State Machines, GDExtension).
- **Beehave** – `bitbrain/beehave` (Behavior Trees in GDScript, Branch `godot-4.x` beachten).
- **Godot State Charts** – `derkork/godot-statecharts` (State Machines; Kern).
- **Terrain3D** – `TokisanGames/Terrain3D` (GDExtension, groß – gut als gebündelt, aber default nicht in jedes 2D-Projekt aktivieren, wenn das Startgeräusch macht; Projekt-Anlage kopiert es, Aktivierung nach Test entscheiden und dokumentieren).
- **ProtonScatter** – `HungryProton/scatter` (3D-Scatter).
- **GUT** – `bitwes/Gut` (Unit-Tests; Lizenz im Repo verifizieren).
- **Aseprite Wizard** – `viniciusgerevini/godot-aseprite-wizard` (Branch `godot_4`; Aseprite selbst ist externe Software – Import funktioniert nur mit installiertem Aseprite, Doku-Hinweis).
- **Debug Draw 3D** – `DmitriySalnikov/godot_debug_draw_3d` (GDExtension, Repo ist sehr groß: nur das Addon vendoren/CI-Build, Lizenz-Verifikation ist Pflicht; wenn die Integration die CI sprengt, als `bundled=optional, default_enabled=false` führen und in `ROADMAP.md` offen vermerken statt so zu tun, als sei es drin).
Optional, wenn schnell & sauber verifiziert: ein Sound-Manager / eine In-Game-Console (nur mit verifizierter MIT-Quelle – **keine URLs erfinden**). Kein Maximalpaket-Pfusch: Lieber die obige Kernliste stabil, als 25 halbdefekte Plugins. Neue Plugins in späteren Versionen nur über `plugins.json` + Test.

### 11. Build & Release (GitHub Actions)

- Leg dich an die vorhandenen Godot-Workflows (`.github/workflows/*`) an und erstelle `.github/workflows/go-build.yml` (`workflow_dispatch` + Tags `go-v*`):
  - Matrix: `windows-2022` + `ubuntu-22.04` (oder jeweils die von der Basis genutzten Runner), Python + SCons wie upstream, Caching (SCons/sccache wie in den Godot-Workflows üblich).
  - Editor-Build (Beispiel, an Basis-Doku anpassen): `scons platform=windows target=editor arch=x86_64 no_editor_splash=no production=yes` bzw. `platform=linuxbsd ...`. Danach Export-Templates (`target=template_debug` + `template_release`) für dieselben Plattformen.
  - GDExtension-Plugins (§10) in der CI für die Zielplattformen bauen bzw. gepinnte Upstream-Release-Zips mit SHA256 prüfen und ins Bundle legen.
  - Artefakt zusammensetzen: Editor-Binary (GO.-Name), Export-Templates in godot-kompatibler Struktur (`.tpz` bzw. Template-Ordner, wie es die Basis erwartet), `go_bundled_plugins/`, `go_project_template/`, `README` (GO.), Lizenz-Sammeldatei (Godot MIT, Kilo/opencode MIT, Monocraft OFL, Plugin-Lizenzen). Artefakte hochladen; bei Tag zusätzlich ein GitHub-Release mit denselben Zips + SHA256-Checksums erstellen.
- Zweiter, **schneller** Workflow (ohne Engine-Compile) für jeden PR: GDScript der gebündelten GDScript-Plugins/`go_project_template` parsen (offizielles Godot-Binary der Basis als Headless-Checker herunterladen ist ok) und `plugins.json` gegen die vendored Ordner validieren (Pfad, `plugin.cfg` existiert, Lizenzdatei existiert). C++-Compile bleibt Sache des großen Workflows (manuell/Tag bzw. Label-gesteuert, um Actions-Minuten zu sparen – in `BUILDING.md` erklären).
- `docs/go/BUILDING.md`: lokale Builds für Windows (Visual Studio 2022, SCons installieren, exakte Befehle) und Linux (Pakete, exakte Befehle), Splash-Hinweis `no_editor_splash=no`, warum eigene Export-Templates nötig sind (`short_name=go`), und was man tun muss, wenn der Build an `splash_editor.png` scheitert (Datei fehlt = GO.-Splash deaktiviert).

### 12. Sicherheit, Lizenzen, Ehrlichkeit

- **Sicherheit:** Keine API-Keys/Secrets im Code, in Tests, in Screenshots oder im Verlauf. Code Agentics sendet nichts ohne User-Absenden. Auto-Commit sendet nur den §8-gekürzten Diff. Git-Credentials bleiben beim System-Git. Keine Telemetrie einbauen. Alle Modell-Ausgaben sind untrusted input (JSON strikt parsen, Längen begrenzen).
- **Tests für die Guardrails sind Pflicht** (als Godot-Test im Modul oder dokumentierte Check-Scripte): `read_file` mit `res://../` schlägt fehl; absoluter Pfad (`C:\...`, `/etc/passwd`) schlägt fehl; `write_file` auf `res://.git/config` schlägt fehl; ausländische Symlink-Ziele werden abgelehnt; Auto-Commit ohne Änderungen macht keinen Commit; Auto-Commit-Fehler entlockt die UI (manuell belegt); Git-Tests gegen ein **lokales Bare-Repo als Fake-Remote** (init → commit → push → clone/status), keine Tests gegen GitHub; Kilo-Client gegen einen lokalen **Mock-HTTP-Server** testen (Model-Filter wirft ein kostenpflichtiges Modell aus einer Fake-`/models`-Antwort heraus und lässt `isFree`-Modelle drin) – in CI kein Echt-Gateway-Call.
- **Lizenzen:** Godot `LICENSE.txt`/`COPYRIGHT.txt` unverändert lassen; GO.-eigener Code unter MIT (Kopfzeile wie upstream-Module es tun). In `docs/go/THIRD_PARTY_NOTICES.md` alles auflisten: Godot, Kilo Code (MIT, opencode-Herkunft), Monocraft (OFL-1.1), jedes Plugin mit Version + Lizenz. Godot-Logo nicht als GO.-Logo ausgeben; „Godot“ nur nominativ („Fork of Godot“) verwenden. „Minecraft“ kommt als Markenwort nur in einem beschreibenden Satz zur Font-Inspiration vor, nicht im Produktnamen und nicht mit Mojang-Assets.
- **Ehrlichkeit im Status:** Führe `docs/go/ROADMAP.md` mit Checkboxen (fertig / teilweise / offen). Nichts als fertig behaupten, was nicht gebaut/getestet wurde (z. B. wenn der Windows-Build nur in CI lief, genau das schreiben). Bekannte Grenzen offen benennen: 200-Req/h-Free-Limit, Modelle ohne Tool-Calling schwächer (Fallback), `editor log`-Zugriff je nach API, macOS offen, C# aus.

### 13. Arbeitsweise (in dieser Reihenfolge, jeder Schritt eigener Commit/PR-Abschnitt)

1. **Inventur:** Upstream-Remote einrichten, Basis-Tag festlegen (§2), Kilo-Referenz clonen + Commit notieren (§3.1), Ist-Struktur (`version.py`, Splash-Dateien, `ProjectDialog`, Editor-Plugin-Beispiele, bestehende Workflows) lesen. `docs/go/`-Gerüst + `BASE_VERSION.md` + `ARCHITECTURE.md` anlegen.
2. **Branding & Splash** (§5) inkl. `make_splash.py`, Fontcommit (§3.3), Version/About/Projekt-Manager. Kurzer Sicht-Test, sobald ein Editor gebaut ist (Screenshot ins PR).
3. **CI-Großworkflow** (§11) früh minimal zum Laufen bringen (erst Editor-Build einer Plattform), damit jede weitere C++-Änderung maschinell geprüft wird.
4. **`modules/go_suite` Grundgerüst:** Modul registriert sich, zwei leere Docks erscheinen/verbergen sich über die Seiten-Buttons, keine Funktion – erst beweisen, dann füllen.
5. **Git Suite** (§6) komplett (erst lesend, dann schreibend) inkl. Host-Erkennung + Remote-Assistent für Codeberg/GitHub/GitLab/Custom (§6.1) und Git-LFS-Manager mit Installations-/Init-Flow, `.gitattributes`-Regeln und Große-Dateien-Erkennung (§6.2), Tests gegen lokales Bare-Remote + LFS-Pointer-Test (§12).
6. **Projekt-Lifecycle** (§7): ProjectDialog-Checkboxen (Git Suite + Git LFS) + Anlage in der §7-Reihenfolge (Init → LFS → Plugins; inkl. Plugin-Kopieren aus §10-Grundstock: Verzeichnis + `plugins.json` jetzt anlegen, Plugins selbst in Schritt 10 füllen), Öffnen-/Schließen-Reminder inkl. LFS-Startprüfung.
7. **Kilo-Client + Code-Agentics-Chat** (§9 ohne Tools): Modellliste laden/filtern, einfacher Chat ohne Aktionen, Abbrechen, Fehlermeldungen (429/offline), UI-Verbote (§1.3) per Sichtprüfung abhaken.
8. **Godot-Tools + Agenten-Schleife** (§9) in Wellen (Dateien → Scripte → Szenen/Nodes → Projekt/Run), Guardrail-Tests, dann der Player-Smoke-Test.
9. **Automatic Commit** (§8) auf Basis von (5) + (7): erst Message-Generator + Staging/Commit, dann Push, dann UI-Lock/Overlay + alle Fehlerpfade; Verdrahtung mit Git-Suite-Button, Schließen-Dialog und dem `auto_commit`-Tool.
10. **Plugins vendern** (§10) eines nach dem anderen (GDScript zuerst), leeres GO.-Projekt-Starten nach jedem Plugin, `PLUGINS.md` fortschreiben; GDExtensions per CI ins Release-Bundle.
11. **Release-Artefakte + Templates** (§11), End-to-End-Test: Mit dem Release-Artefakt ein neues Projekt (Git Suite AN) anlegen, Szene per Code Agentics bauen lassen, Automatic Commit gegen ein Test-Repo, Projekt exportieren (Template-Test).
12. **Doku & Feinschliff:** Haupt-README (GO.-Abschnitt oben, Godot-Original darunter verlinkt – Upstream-README nicht löschen, sondern z. B. als `README.godot-upstream.md` bewahren, wenn das Haupt-README ersetzt wird), `ROADMAP.md` wahrheitsgemäß ausfüllen, Changelog `docs/go/CHANGELOG.md` beginnen.

Commit-Disziplin: Conventional Commits (`go: …`, `go-git: …`, `go-agentics: …`, `go-plugins: …`, `docs(go): …`), Godot-Code-Style einhalten (Tabs/`clang-format` wie upstream, vor C++-Commits ggf. `misc/scripts`-Format-Checks des Repos laufen lassen). Niemals Build-Artefakte (`.o`, `.scons*`, `bin/`), `.godot/`-Caches, Kilo-`node_modules` oder Download-Zips committen (`.gitignore` entsprechend ergänzen, GO.-markiert).

### 14. Definition of Done (der Reihe nach belegen, nicht nur behaupten)

- [ ] Windows- und Linux-Editor bauen in GitHub Actions grün; `--version` zeigt „GO. Game Engine“.
- [ ] Editor-Splash zeigt GO.-Logo + unten rechts `A Godot Game Engine Fork by SampleCraft` in Pixel-Font (Screenshot).
- [ ] Projekt-Manager trägt GO.-Branding; About nennt GO. als Godot-Fork mit Credits.
- [ ] „Neues Projekt“ mit Git-Suite-Häkchen: Projekt hat `.git/` (Branch `main`), `go/git_suite/enabled=true`, gebündelte Kern-Plugins kopiert+aktiviert, GO.-Boot-Splash. Ohne Häkchen: kein `.git/`, keine Reminder.
- [ ] Git-Dock: Status/Stage/Commit/Pull/Push/Log gegen ein lokales Bare-Remote durchgespielt (Beleg im PR-Text).
- [ ] Öffnen-Reminder erscheint bei behind-Remote und bei uncommitteten Änderungen, sonst nicht. Schließen-Reminder erscheint bei uncommittet/unpusht, sonst nicht. „Nicht mehr anzeigen“ wirkt.
- [ ] `Automatic Commit`: Overlay sperrt die UI (ein Klick/Tastenschlag dahinter ist wirkungslos – manuell belegen), Progress zeigt die Etappen, Commit-Message kommt nachweislich aus `kilo-auto/free` (Log), Commit+Push landen im Test-Remote, UI wird danach wieder frei. Fehlertest: Push-Rejektion entlockt die UI und meldet klar.
- [ ] Code Agentics: Picker zeigt nur Free-Modelle (Fake-Katalog-Test + einmal Live-Sichtprüfung), kein Provider/MCP/Login-/Workspace-UI-Element vorhanden (Screenshots des ganzen Docks), Workspace-Zeile gesperrt auf das Projekt.
- [ ] Player-Smoke-Test (§9) läuft per Agent durch; Guardrail-Tests (§12) sind grün/dokumentiert.
- [ ] Leeres neues GO.-Projekt mit den default-aktiven Plugins öffnet ohne Fehler/ Warnungsflut; Projekt mit GO.-Templates exportierbar (Beispiel-Spiel startet).
- [ ] Alle `docs/go/*.md` vorhanden und ehrlich; `UPSTREAM_PATCHES.md` listet jede Upstream-Datei-Änderung; Release-Zip (§11) lässt sich auf einer sauberen Maschine (ohne Godot-Vorinstallation, ohne Netzwerk für die Einrichtung) entzippen → Editor startet → neues GO.-Projekt funktioniert.

### 15. Wie du mir (dem User) berichtest

- Nach jedem der Schritte aus §13 eine kurze Meldung auf Deutsch: Was ist fertig, welcher Commit, wie wurde es getestet, was ist als Nächstes dran. Screenshots bei allen UI-Schritten (Splash, beide Docks, Overlay, Reminder, Projekt-Dialog).
- Am Ende eine Abschluss-Zusammenfassung auf Deutsch: Erfüllte Punkte aus §14 (mit Belegen), ehrliche Restpunkte aus `ROADMAP.md`, die exakten Download-Links/Schritte, um ein Release zu bauen, und wie ich den Fork später mit neuem Godot-Upstream synchron halte.
- Wenn die Ausführungsumgebung einen Schritt nachweislich nicht kann (z. B. Godot lokal compilieren bei zu wenig RAM/CPU), nicht so tun als ob: Schritt in die CI verlagern, das Ergebnis von dort belegen und die Einschränkung offen in `ROADMAP.md` festhalten.

Beginne jetzt mit Schritt 1 aus §13 und arbeite dich durch. Viel Erfolg – bau die GO. Game Engine.

## PROMPT-ENDE
