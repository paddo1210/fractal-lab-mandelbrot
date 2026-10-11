# FRACTAL LAB – MANDELBROT v0.2b GUI

Windows-x64-VST3 erfolgreich gebaut; alle vier automatisierten Tests bestanden. [Installierbare ZIP herunterladen](https://github.com/paddo1210/fractal-lab-mandelbrot/releases/download/v0.2b-preview/FRACTAL-LAB-MANDELBROT-v0.2b-Windows-x64-VST3.zip). [Build #2](https://github.com/paddo1210/fractal-lab-mandelbrot/actions/runs/38156945057), getesteter Commit `45261520b2c9bcec991ab2777ba8917289ac1156`. Praktische Ableton-/APC20-Abnahme und visuelle Laufzeitprüfung bleiben offen. Siehe TEST_REPORT.md.

ZIP entpacken und den vollständigen Ordner `FRACTAL LAB MANDELBROT v0.2b.vst3` nach `C:\Program Files\Common Files\VST3` kopieren, danach in Live neu scannen. Die neue Kennung erhält v0.2a als separates Plugin. Im Repository sind die normalen Quellen beziehungsweise das GitHub-Quellarchiv des Release-Tags maßgeblich; FractalLab-v02b-source.zip dokumentiert den ursprünglichen Import vor der Compilerkorrektur.

## Oberfläche und Architektur

PluginEditor.h ersetzt den bisherigen Generic-Editor als Hauptoberfläche. Native JUCE-Regler verwenden APVTS-Attachments: X/Y, beide Zoomparameter, Radius, drei Amounts, Gate, Root, Basisvelocity, Scale, Mode, Sampler, Iterationen, Run/Panic und alle 16 Degrees, On-Schalter und CTRL-Velocities. Weitere Parameter sind über „All parameters / diagnostics“ erreichbar. Keine vorgetäuschten Pattern-Speicher, Output-Meter oder Kanalumschaltung: Die bestehende Engine bietet diese Funktionen nicht.

Der Porträtbereich wird direkt aus Assets/DesignReference.png gezeichnet. Bedienelemente sind native Komponenten, keine klickbaren Bereiche auf dem Entwurfsbild. Dunkle Panels und Cyan-/Orange-Akzente übernehmen die visuelle Referenz. Größe 1200 × 1020; Layout und DPI-Skalierung noch praktisch prüfen.

Designverfeinerung nach der ergänzenden Elementübersicht (Assets/StyleReference.png): LabTheme.h bündelt die exakten Farbfelder Cyan #00E5FF, Blau #0088FF, Orange #FF9A3C, Rot #FF3D57 sowie Hintergrund-, Panel-, Stroke- und Textfarben. Native Regler erhalten Skalenstriche, Metallverlauf, Hover- und Disabled-Zustände; Buttons/Step-Schalter klare Zustandsrahmen; vertikale Fader farbige Werte und Griffpunkte. Die 16 Step-Spalten sind einzeln gerahmt. Panic verwendet den roten Akzent. Keine neue Schriftdatei wurde geliefert; die JUCE-Systemschrift bleibt erhalten. Die ergänzende Übersicht dient als Referenz und ist kein Satz separater transparenter PNGs.

FractalView berechnet 944 × 208 Pixel auf einem eigenen JUCE-Thread. Neue Aufträge ersetzen ausstehende; der Worker prüft Abbruch/Änderungen nach jeder Zeile. Bild und zugehörige Settings werden gemeinsam unter einem GUI-/Worker-Mutex übergeben. Der Audiothread verwendet diesen Mutex nicht. Beim Schließen wird der Worker beendet und gejoint.

ViewMath.h: z₀=0, zₙ₊₁=zₙ²+c; Flucht bei |z|²>4. Schwarz bedeutet „bei N nicht geflohen“, keinen allgemeinen Mengenbeweis. Gleiche Skalierung auf Real-/Imaginärachse, Y zeigt nach oben; Mittelpunkt X/Y, vertikale Ausdehnung 3/effektivem Zoom. Alle 16 Markierungen nutzen unmittelbar fractal::coordinate mit Radius/Zoom und Kreis-/Linienmodus. Effektiver Zoom = Multiplikator × 10^Exponent. Float-Hostparameter bleiben die Präzisionsgrenze.

Das Fraktal zeigt aktuelle Reglerwerte als PREVIEW. Scrollbare Diagnostik zeigt das übernommene Audiopattern mit der bisherigen Einschränkung unabhängiger atomarer Updates. Übernahme am nächsten Zyklus unverändert. Kein erfundener Transport-/Step-Cursor.

## APC20 über den Host

Keine direkte Hardware-Anbindung und kein neues MIDI-Learn im Plugin. Parameter-IDs, Bereiche und Versionsnummern bleiben unverändert. MIDI-Engine, Eingangslöschung und Ausgabe auf Kanal 1 bleiben erhalten.

Vorschlag für acht Host-Makros/Fader: x, y, zoomExponent, radius, pitchAmount, rhythmAmount, velocityAmount, gate. Weitere Host-Bänke können step01…step16, vel01…vel16 und on01…on16 nutzen. Konkrete Host-Konfiguration und vorhandene APC20-Scripts müssen praktisch geprüft werden. LED-Rückmeldung wird nicht behauptet.

## Separate Rückfallversion

v0.2a wurde nicht geändert. Neue Variante: Produktname FRACTAL LAB MANDELBROT v0.2b, Plugin-Code MbMb, Bundle-ID org.fractallab.mandelbrotmidi.v02b. v0.2a behält MbMa und seinen Produktnamen. State-Typ/Schema 2 unverändert; paralleles Laden und State-Übertragung noch im Host testen. Kopie des ursprünglichen v0.2a-Quellarchivs liegt neben diesem Projekt. Installierte VST3-Dateien wurden nicht angefasst.

## Build

Windows: Visual Studio 2022, MSVC v143, Windows SDK, CMake ≥3.22, JUCE 8.0.6.

```powershell
cmake --preset windows-vs2022
cmake --build --preset release --parallel 4
ctest --preset release --verbose
```

Offline JUCE über -DJUCE_SOURCE_DIR=<lokaler JUCE-8.0.6-Pfad> bereitstellen. VST3-Ziel: build/FractalLabV02b_artefacts/Release/VST3/FRACTAL LAB MANDELBROT v0.2b.vst3.

CTest enthält Engine-, Processor-, ViewMath- und GUI-Attachment-Tests. Alle vier wurden im Windows-Release-Build #2 erfolgreich ausgeführt. Die Veröffentlichung ist als Preview für die noch offene Ableton-/APC20-Abnahme markiert.
