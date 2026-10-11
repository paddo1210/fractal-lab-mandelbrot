# v0.3 – Phase 1/2: Analyse und technisches Konzept

Ausgangspunkt: lokal erhaltene v0.2b, gebaut in GitHub-Lauf 38156945057. Separate neue Identität: MbMc / org.fractallab.mandelbrotmidi.v03 / FRACTAL LAB MANDELBROT v0.3; Hauptbranch und installierbares v0.2b bleiben erhalten.

## Bestandsanalyse

v0.2b: 72 APVTS-Parameter, Generic-Detailansicht plus eigener Editor; 16 Sechzehntel pro vier PPQ-Quarterbeats, 4/4-Annahme. Engine::process berechnet makePattern am Start/Seek und an Vier-Beat-Grenzen synchron. Audio wird gelöscht; Eingangsmidi wird gelöscht; monophone Note-On/Off-Ausgabe auf Kanal 1. GUI-Fraktalrechnung läuft bereits separat, die musikalische Bahnanalyse bisher nicht. Atomare Diagnostik ist kein transaktionales Pattern-Snapshot. X/Y sind floatbasierte Hostparameter; Bahnanalyse double.

## Phase-3-Entscheidung

Eine beliebige Live-Neuberechnung auf einem Worker mit „bei Verspätung altes Pattern spielen“ kann zwischen Echtzeit- und Offline-Läufen unterschiedliche MIDI-Ausgaben erzeugen. Deshalb verwendet der minimale Prototyp einen vollständig vorbereiteten Flugplan. Es werden keine Kandidaten während der Wiedergabe nachgeladen. Der Musiker wartet auf Abschluss von PREPARE bei gestopptem Transport. PREPARE beziehungsweise Preset-Laden sind nicht-realtime Kontrolloperationen; nie im processBlock. Die Berechnung erfolgt auf einem Preparation-Worker, dessen Abschluss auf der Kontrollseite abgewartet wird. Ein während PREPARE gestarteter Transport ist keine unterstützte Abnahmebedingung dieses Prototyps.

Ein Bank enthält 2048 echte Mandelbrot-Analysen mit jeweils 16 Punkten. Samplephase ist floor(2048*phase)/2048. Keine Interpolation von Merkmalswerten, keine erfundenen Fluchtzeiten. GUI und Kandidaten verwenden die tatsächlich analysierten quantisierten Koordinaten. Navigation ist fein gerastert, nicht unendlich kontinuierlich. Mapping der gespeicherten Merkmale in Noten/Velocity/On bleibt im Callback begrenzt auf feste 16 Steps; die teuren Orbits liegen außerhalb. Das bestehende makePattern bleibt für Tests und Vorbereitung verfügbar, wird vom v0.3-processBlock nicht verwendet.

Eine periodische Flugbahn hat 256 PPQ-Beats, bei Speed=1 also 64 Takte in 4/4. Phase ist hostzeitgebunden; Speed integriert Stück für Stück, Freeze/Flight-Off halten die Phasenuhr. Bei Seek wird die Phase aus absoluter PPQ rekonstruiert, außer bei Freeze. Seek setzt das musikalische Pattern auf den Capture-Bezug beziehungsweise den vorbereiteten Ursprung zurück. Ohne gleiche Automationshistorie besteht kein Versprechen, am Seek-Ziel die während einer früheren Wiedergabe entstandene Variation wiederherzustellen.

Mit a=2πphase, w=(1−cos a)/2: X=X₀+0,1*DriftX/Zoom₀*sin a; Y=Y₀+0,1*DriftY/Zoom₀*(1−cos a). Zoom In: log10(Zoom)=log10(Zoom₀)+(4−log10(Zoom₀))*w. Zoom Out: log10(Zoom)=log10(Zoom₀)*(1−w). Bahn kehrt weich zum Ursprung zurück. Clamping auf bestehende X/Y-Grenzen kann an Randwerten die Ableitung brechen. Bei Zoom₀=1 kann „Out“ nicht weiter herauszoomen.

Zoom ist auf 1…10⁴ begrenzt. Pixelgröße bei 104 Rechenzeilen ist mindestens 3/(104*10⁴)≈2,88e−6 und damit weit größer als Double-ULPs im betrachteten Koordinatenbereich. Die Float-Hostkoordinaten begrenzen weiterhin die Wahl des Ausgangspunktes; Double-Flugkoordinaten werden anschließend nicht zurück in Float-Parameter geschrieben. Keine Deep-Zoom-Behauptung. Für wesentlich tiefere Zooms wäre ein hochpräziser Referenzorbit mit Perturbation und Fehlerkontrolle nötig; hier nicht implementiert.

## Controlled Evolution und Speicher

Evolution-Zeitpunkte liegen auf absoluten PPQ-Grenzen von 16/32/64 Beats (4/8/16 Takte). 16-Step-Pattern wiederholt sich unverändert zwischen diesen Grenzen. Default 8 Takte / SUBTLE. Mutation OFF/1/2 begrenzt die Zahl unterschiedlicher Step-Tupel (Pitch, Velocity, On). Änderungen aller drei Felder zählen als ein Step. Priorität: Summe der absoluten Unterschiede von Magnitude, Movement, Contrast und EscapeIndex/512. Gleichstand: niedrigerer Stepindex. Feste Insertion-Sort, keine Zufallszahl, keine Allokation. Gate ist ein Patternwert, folgt nur bei aktiver Mutation einer zugelassenen Grenze.

CAPTURE speichert das tatsächlich angewandte Pattern; HOLD spielt den Capture-Bezug, ohne Capture hält es das aktuelle Pattern. EVOLVE OFF sperrt Veränderungen. RECALL und RESET A werden an der nächsten Vier-Beat-Grenze angewandt. FLIGHT RESET setzt dort die Phasenuhr auf den vorbereiteten Ursprung. Aktionstasten verwenden einen Wechsel des booleschen Hostparameters als Kommando; gespeicherte Kommandozustände werden beim Preset-Laden nicht erneut ausgeführt.

Der Speicher enthält tatsächliche MIDI-Tupel, Gate, die berechneten Merkmale, ursprüngliche/mutierte Abtastpunkte sowie mathematische und Mapping-Settings. Ein durch Mutation zusammengesetztes Pattern stammt nicht zwangsläufig aus einer einzigen Fraktalposition; seine per-Step-Punkte bleiben separat erhalten. „Math“ beschreibt die zuletzt geprüfte Kandidatenkonfiguration, nicht einen Beweis für einen einzigen Gesamtorbit des gespeicherten Motivs. Der vorbereitete Ursprung und die Driftkonfiguration werden unabhängig von eventuell noch nicht vorbereiteten Regleränderungen gespeichert. MIDI-Recall ist implementiert; separate Übernahme des Captures als neue Navigationsbasis gehört noch nicht zum Prototyp.

Feste Doppelbuffer mit atomaren Leserschutz-Zählern verbinden Vorbereitung, Audio und GUI. Audio wartet nicht auf Mutex/Worker und allokiert keine Bank. GUI-Bilder haben eine eigene Worker-Pipeline; sie beenden ein Bild trotz neuer Anfragen, damit schnelle Bewegung das Rendering nicht dauerhaft verhungern lässt. Veraltete vollständige Bilder sind zulässige visuelle Latenz; MIDI hängt nicht von ihnen ab.

## Bewusste Grenzen

Navigationseinstellungen X/Y/Zoom/Radius/Sampler/Iterationen/Drift/Zoomrichtung sind im laufenden Bank vorbereitet. Änderungen an diesen Hostparametern sind vorgemerkt und benötigen Stop → PREPARE → Play. Pitch/Rhythm/Velocity, Root/Scale/Register und Basissteps werden aus den bestehenden Hostparametern an Evolution-Grenzen musikalisch abgebildet. Identische vorbereitete Ausgangszustände und identische eingelesene Automationswerte ergeben dieselbe MIDI-Ausgabe. Der JUCE/APVTS-Wrapper liest Automation weiterhin blockweise; keine neue Zusage samplegenauer Automation oder beliebiger Puffergrößenunabhängigkeit bei innerhalb eines Blocks geänderten Parametern.

Nur 4/4 ist unterstützt; vorhandene andere Host-Taktsignaturen werden sicher stumm geschaltet. Fehlende Signaturinformationen werden als 4/4 behandelt. VISUAL FLIGHT, EXPLORE, A/A′, A/A/A′/A, weitere Evolution-Raten/Mutationstiefen und Math-Recall sind Phase-5-Erweiterungen, keine erfolgreich implementierten Funktionen.

## APC20-Bestandsprüfung

Die sichtbaren User Remote Scripts unter den lokalen Live-11.0.2-/11.3.43-Preferences enthalten nur Standardvorlagen; Pad-/Encoder-Zuordnungen stehen auf −1. Das belegt nicht, dass die APC20-Pads frei sind: installierte Hersteller-Scripts und MIDI-Mappings des geöffneten Live-Sets wurden nicht ausgelesen. Deshalb keine feste Padbelegung und kein Script geändert. Fader 1…7 behalten x,y,zoomExponent,radius,pitchAmount,rhythmAmount,velocityAmount. Fader 8 wird nicht vorausgesetzt. Neue Hostparameter können nach Prüfung des tatsächlichen Live-Sets auf freie Pads gelegt werden.
