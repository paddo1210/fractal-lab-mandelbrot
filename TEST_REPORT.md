# v0.3 Prüfbericht

Status: v0.3-Phase-3-Prototyp erfolgreich als Windows-x64-VST3 gebaut; alle fünf Testprogramme bestanden. Keine behauptete Ableton-/APC20-Abnahme.

## Erfolgreicher Build und Auslieferung

[GitHub-Lauf #6](https://github.com/paddo1210/fractal-lab-mandelbrot/actions/runs/38170180777). Tatsächlich getesteter Quellcommit nach Import: `998d903080d5b424feb198cba3e2fb447938ab2a`; Release-Tag v0.3-prototype zeigt auf diesen Commit. Abschluss am 11. Oktober 2026 um 22:04 Uhr, Europe/Berlin. Windows Server 2022 x64, MSVC 19.44, JUCE 8.0.6, Release.

| Testprogramm | Ergebnis | Laufzeit |
|---|---|---|
| engine_tests | bestanden | 0,11 s |
| processor_tests | bestanden | 0,42 s |
| gui_tests | bestanden | 0,20 s |
| view_math_tests | bestanden | 0,01 s |
| evolution_tests | bestanden | 0,09 s |

CTest: 5/5, 0 Fehler; insgesamt 0,88 s. Processor-Test prüft tatsächliche JUCE-nonRealtime-Flags, 32 Takte MATH FLIGHT, 127er versus 4096er Blockgröße, identische MIDI-Bytes/Samplepositionen und exakte Capture-Preset-Roundtrips für Noten, Koordinaten, Ranking-Merkmale und Navigation. GUI-Test prüft neue Flight-/Freeze-/Rate-/Depth-/Speed-Bindungen zusätzlich zu den bestehenden X- und 16-Step-Bindungen. Vorhandene Engine-Tests decken Stop, Loop/Seek, Panic, ungültiges Tempo, Reset und Sample-Rate-Wechsel ab.

Evolution-Test: A 32-Takt-Hold bei weiterlaufendem Flug, B nur Acht-Takt-Grenzen, C maximal ein Step, D wiederholte MIDI-Folgen, F Freeze, H gepaarte Note-On/Off-Bilanz und I unterschiedliche Blockgrößen im Engine-Harness bestanden. Außerdem LOW, feste Gleichstandsregel, Flight Stop, Zoomgrenzen und mathematische Analyse bei leerem musikalischem Register geprüft. E (A/A/A′/A) und G (VISUAL FLIGHT) sind nicht implementiert und ausdrücklich nicht bestanden/ausgeführt. Die Offline-Prüfung ist automatisierter Prozessor-/Engine-Harness, kein Ableton-Offline-Export.

[Veröffentlichte VST3-ZIP](https://github.com/paddo1210/fractal-lab-mandelbrot/releases/download/v0.3-prototype/FRACTAL-LAB-MANDELBROT-v0.3-Windows-x64-VST3.zip) lokal heruntergeladen und geprüft: vollständiger Bundlepfad, Archiv-CRC, sichere Pfade, Windows-x64-PE-DLL, INSTALL.txt und übereinstimmende SHA256-Datei. Binärgröße 8.826.368 Bytes. ZIP-SHA256 `4d9a5378e0dfbafd0c5e900a2f6cfb134ccc1a5b42cb3428e2f0a5607f31cbee`; Binär-SHA256 `d924082afee7c135cf7434bac5613d2a411f64f3cd7eb975cfa6e957d0910f16`. Lokale maschinenlesbare Paketprüfung: V03-PACKAGE-CHECK.json im übergeordneten outputs-Ordner.

v0.2b wurde nicht geändert oder installiert/überschrieben. GitHub-main und der bestehende v0.2b-Release bleiben erhalten. v0.3 verwendet MbMc und einen eigenen Produkt-/Bundlenamen. Paralleles Laden in Ableton ist praktisch noch zu prüfen.

Zwischenergebnisse: Lauf 38168217311 scheiterte am Import vorhandener Ordner; Merge mit Copy-Item -Force lokal geprüft. Lauf 38168348928 kompilierte erfolgreich, Processor-Test scheiterte am exakten Roundtrip mathematischer Punkte. Verlustfreie Double-Speicherung korrigiert, Test nicht abgeschwächt. Überholter Lauf 38170151598 abgebrochen; der vollständige korrigierte Lauf #6 ist oben dokumentiert.

Bekannte Grenzen: 2048 Positionen auf periodischer 64-Takt-Bahn bei Speed 1, Zoom bis 10⁴, nur 4/4 und Phase-3-Modi. Navigation erfordert Stop → PREPARE → Play; Transportstart während PREPARE und Preset-Laden bei laufendem Transport sind keine unterstützten Abnahmebedingungen. Hostautomation bleibt blockweise; beliebige innerhalb eines Blocks geänderte Automation ist nicht durch den statischen Blockgrößenvergleich abgedeckt. Uniforme Fraktalregionen werden nicht künstlich mit Strukturen aufgefüllt. Gate ist ein globaler Pattern-Timingwert außerhalb der gezählten Step-Tupel (Pitch/Velocity/On).

Ableton-/APC20-Prüfung, visuelle Flugabnahme, CPU-Messung und Host-Automation stehen aus. Grenzen: TECHNICAL_CONCEPT.md.
