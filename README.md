# FRACTAL LAB MANDELBROT v0.3 – Phase-3-Prototyp

Separate VST3-Identität MbMc; v0.2b bleibt im main-Branch und als bisherige ZIP erhalten.

Implementiert: vorbereiteter MATH FLIGHT, Speed, Drift X/Y, Zoomrichtung, Freeze, Flight Reset, Capture/Hold/Evolve/Recall/Reset, 4/8/16-Takt-Evolution, Mutation OFF/SUBTLE/LOW, gespeicherte MIDI- und mathematische Provenienz, neue GUI, 87 Hostparameter. Default: 8 Takte, maximal ein Step. Windows-Release-Build und alle fünf automatisierten Testprogramme bestanden; siehe TEST_REPORT.md.

[Installierbare Windows-VST3-ZIP](https://github.com/paddo1210/fractal-lab-mandelbrot/releases/download/v0.3-prototype/FRACTAL-LAB-MANDELBROT-v0.3-Windows-x64-VST3.zip) · [Erfolgreicher Build](https://github.com/paddo1210/fractal-lab-mandelbrot/actions/runs/38170180777). Quellcommit: 998d903080d5b424feb198cba3e2fb447938ab2a. Praktische Ableton-/APC20- und visuelle Abnahme offen.

Start: X −0,75 / Y 0,1 / Zoom 1 / Radius 0,37 / N=128; Rhythm MASK=0; Pitch/Velocity=0,5. Transport stoppen, PREPARE drücken und Abschluss abwarten. Flight aktivieren, MATH FLIGHT auswählen, Speed=1, EVOLVE an. Nach Wiedergabe CAPTURE und HOLD, um das Motiv bei weiterlaufendem Flug zu wiederholen. Navigationseinstellungen benötigen nach Änderungen erneut Stop → PREPARE → Play. Capture/Recall/Reset lösen bei jedem Wechsel ihres booleschen Hostparameters aus.

Bahn: 2048 echte analysierte Positionen auf einem periodischen 64-Takt-Flug bei Speed 1. Zoom 1…10⁴. Kein unbegrenzter Deep Zoom, kein Zufall und keine erfundene Fluchtzeitinterpolation. Grafik zeigt aktuelle Kandidatenpunkte; gespeicherte Musik kann aus älteren/gemischten Zuständen stammen.

Nicht implementiert: VISUAL FLIGHT, EXPLORE, A/A′ und A/A/A′/A, Raten 1/2/32/64, Tiefen MEDIUM/HIGH/FULL und separate Wiederherstellung der Capture-Navigation. Nur 4/4. Blockweise Hostautomation bleibt eine Einschränkung; Start während PREPARE nicht unterstützt.

APC20: Fader 1…7 behalten x,y,zoomExponent,radius,pitchAmount,rhythmAmount,velocityAmount. Kein achter Fader und keine automatische Padbelegung. Zugängliche User-Script-Vorlagen gelesen; aktuelle Hersteller-Scripts/Live-Set-Belegungen nicht verifiziert. Keine Konfiguration überschrieben.

Technik: TECHNICAL_CONCEPT.md. Keine behauptete Ableton-/APC20-Abnahme.

Build: VS2022/MSVC/Windows SDK/CMake ≥3.22, JUCE 8.0.6; cmake --preset windows-vs2022, cmake --build --preset release --parallel 4, ctest --preset release --verbose.

Installation nach erfolgreichem Build: ZIP entpacken, vollständigen FRACTAL LAB MANDELBROT v0.3.vst3-Ordner nach C:\Program Files\Common Files\VST3 kopieren, Live neu scannen. MIDI-Ausgabe auf Kanal 1 zu einem Instrument routen; Audio bleibt stumm. v0.2b behalten.
