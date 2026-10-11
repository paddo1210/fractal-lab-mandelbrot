# Prüfbericht – 11. Oktober 2026

Status: Windows-x64-Release-Build erfolgreich; alle vier CTest-Tests bestanden. Praktische Ableton-/APC20-Abnahme und visuelle Laufzeitprüfung offen.

## GitHub-Build und Paketprüfung

Repository: https://github.com/paddo1210/fractal-lab-mandelbrot

Erfolgreicher Lauf #2: https://github.com/paddo1210/fractal-lab-mandelbrot/actions/runs/38156945057

Getesteter Commit: 45261520b2c9bcec991ab2777ba8917289ac1156. Windows Server 2022 x64, MSVC 19.44, JUCE 8.0.6, Release. Abschluss am 11. Oktober 2026 um 18:54 Uhr (Europe/Berlin).

- engine_tests: bestanden (0,14 s).
- processor_tests: bestanden (0,03 s).
- gui_tests: bestanden (0,21 s), einschließlich mehrfacher Editor-Lebensdauer und Parameterbindungen in beide Richtungen.
- view_math_tests: bestanden (0,01 s).

4/4 Tests bestanden; Gesamtlaufzeit laut CTest 0,44 s. JUCE erzeugte das vollständige VST3-Bundle einschließlich moduleinfo.json. Der Build verifizierte die erwartete Binärdatei vor ZIP-Erstellung. Installierbares Paket als Release v0.2b-preview veröffentlicht.

Erster GitHub-Lauf scheiterte an der falschen Override-Signatur für drawButtonBackground (TextButton& statt Button&). Nach Abgleich mit dem JUCE-8.0.6-Header korrigiert; vollständiger Build erneut ausgeführt. Keine Engine-/Routingänderung.

Die veröffentlichte ZIP wurde heruntergeladen und lokal geprüft: Archiv-CRC, sichere Pfade, vollständiger Bundlepfad, Windows-x64-PE-DLL-Header, INSTALL.txt und übereinstimmende SHA256-Datei. Binärgröße 8.776.192 Bytes. ZIP-SHA256: f82a85654c13fe5740d3d8c3316cdc70b76f4e21163f15b8114d97dbcd6fe70a. Binär-SHA256: a0ba96668bd26c47e911f7e5aa5d095cc53a42859ce437a7f82ffd8ea72dbe1c. Maschinenlesbare lokale Paketprüfung: ../VST3-PACKAGE-CHECK.json. Kein lokaler Hoststart.

Designrevision: Farbpalette und native LookAndFeel-Zeichnung nach ergänzender Elementübersicht angepasst. Skalenstriche, Hover-/Disabled-Zustände, Button-/Toggle-Zustände, Fader und Step-Rahmen hinzugefügt. Keine Änderung an Engine, Parameterlayout, MIDI-Routing oder Fraktalmathematik. Visuelle Qualität im laufenden JUCE-Editor noch nicht geprüft. Der folgende Quellintegritätscheck wurde nach dieser Revision erneut ausgeführt.

## Tatsächlich ausgeführt

Python-Quellintegritätsprüfung: 8/8 bestanden, finaler Prozess-Exitcode 0. Ergebnis: SOURCE_CHECKS.json.

- Engine.h byteidentisch zu v0.2a.
- processBlock, Parameterlayout und settings unverändert.
- State-Serialisierung unverändert (abschließende Leerzeilen ignoriert).
- Eigene Plugin-Kennung vorhanden.
- createEditor erzeugt den neuen JUCE-Editor.
- Eingebettetes Designasset vorhanden.

Das sind statische Prüfungen, kein Beleg für Kompilierbarkeit, GUI-Bindungen zur Laufzeit oder mathematische C++-Testergebnisse.

## Früherer lokaler Buildversuch / Umgebung

CMake, Ninja, MSVC und Clang/GCC wurden in den geprüften lokalen Werkzeugpfaden nicht gefunden. Ältere Zig-Installation nicht ausführbar/zugänglich. Downloadversuch der Buildwerkzeuge über pip/PyPI fehlgeschlagen („No matching distribution found“). Die anschließend angeforderte Eskalation wurde durch die automatische Freigabeprüfung abgelehnt: Sandbox-Eskalationen sind in dieser Sitzung deaktiviert. Die vorhandene JUCE.zip ist kein gültiges ZIP-Archiv.

Lokal konnte CMake nicht gestartet werden. Dieser frühere Befund wurde durch den oben dokumentierten GitHub-Windows-Build ergänzt. Frühere v0.2a-CI-Ergebnisse wurden nicht als Tests dieser GUI-Version gewertet.

## Ausgeführte Tests

- ViewMathTests.cpp: bekannte Flucht-/Nichtfluchtpunkte, Y-Orientierung, Zentrum, 16 Koordinaten-Roundtrips für Kreis/Linie bei Zoom 1/10/10000.
- GuiTests.cpp: Host → X-Regler, X-Regler → Hostparameter, 16 Degree-/Velocity-/On-Bindungen, wiederholtes Erzeugen/Zerstören des Editors.
- Bestehende EngineTests.cpp und ProcessorTests.cpp bleiben enthalten.

## Erforderliche Abnahme

1. Windows-Release-Build und alle vier CTest-Ziele: erledigt, siehe oben.
2. GUI: Porträtzuschnitt, Text-/Reglerlayout, DPI-Skalierung und Zielbildschirm.
3. Host: Automation, Speichern/Laden, paralleles Laden v0.2a/v0.2b.
4. APC20: reale Host-Makrozuweisungen, Step-Bänke und Wertebereiche; LED-Rückmeldung ungetestet.
5. Schnelle Navigation bei N=512, Editor-Lebensdauer, Worker-Abbruch, GUI-/Audio-CPU und MIDI unter Last.
6. Fraktal/Koordinaten gegen bekannte Punkte prüfen; Vorschau vom am Zyklus übernommenen Audiopattern unterscheiden.

Keine offenen Prüfungen werden als erfolgreich angegeben.
