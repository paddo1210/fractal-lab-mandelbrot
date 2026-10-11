# MANDELBROT v0.2 — Analyse und mathematisches Konzept

Stand: 11.10.2026. Phase 1 und 2, mit vorgeschlagener Architektur zur Diskussion. Keine Implementierung von v0.2. Grundlage: lokaler v0.1-Code einschließlich MIDI-Eingangsbus-Korrektur, die auf GitHub als Commit 6d988e2 veröffentlicht wurde. Der Nutzer bestätigt das funktionierende Zwei-Spur-Routing; eine eigene vollständige Live-Abnahme liegt nicht vor.

## 1. Was v0.1 tatsächlich berechnet

Für Step s=0…15: c_s=X+iY+(0.02+0.7A)exp(2πis/16). Jede Bahn startet bei z_0=0; z_(n+1)=z_n²+c_s. Abbruch nach dem ersten n mit |z_n|²>4, maximal 96 Iterationen. v=n/96 bei Flucht, ansonsten v=1.

GEN verwendet Degree=floor(27v). CTRL verwendet den gespeicherten Step-Degree plus round((2v−1)7A). Die Stufe wird in Chromatik, Dur, Moll oder Mollpentatonik übersetzt. Velocity=clamp(Basis+round((2v−1)30A),1,127).

Die Mandelbrot-Iteration ist korrekt. Die musikalischen Regeln — hohe Fluchtzeit als hohe Note, Kreis als Sequenzreihenfolge, Skala und Velocity — sind entworfene Interpretationen. Sie sind keine unmittelbar in der Menge enthaltenen musikalischen Gesetzmäßigkeiten.

Timing: Step=floor(4·PPQ), Index modulo 16. Vier Viertel pro Zyklus, feste Sechzehntel, auch bei anderer Taktart. Step-On schaltet Pausen. Gate bestimmt einen Anteil von 0.25 Vierteln. Der monophone Scheduler setzt Note-Off vor einer neuen Note, bei Stop, Run/Panic und erkannten Positionssprüngen. Ohne weitere processBlock-Aufrufe, etwa nach Plug-in-Entfernung, kann er keine ausstehenden Note-Offs mehr liefern.

42 Parameter werden blockweise gelesen und als APVTS-Zustand gespeichert. Keine eigenen benannten Presets oder Positionsspeicher, kein Zufall, keine selbstständige zeitliche Navigation. GEN wiederholt sich. Der MIDI-Eingang ist für Live exponiert, seine Ereignisse werden verworfen. Audio bleibt stumm.

## 2. Einschränkungen und mögliche Fehler

| Befund | Bedeutung / vorgeschlagene Behandlung |
|---|---|
| Kein Escape nach 96 wird als v=1 ausgegeben | Kein allgemeiner Zugehörigkeitsbeweis. In v0.2 als „bis N nicht geflohen“ kennzeichnen. |
| Alle solchen Bahnen werden gleich gemappt | Fixpunkt, Zweierzyklus, präperiodische Bahn und langsame spätere Flucht werden musikalisch zusammengeworfen. Orbitmerkmale ergänzen. |
| Amount verändert Geometrie, Pitch und Velocity gleichzeitig | Ursachen beim Hören schwer trennbar. Radius, Zoom und drei Amounts separieren. |
| Division der Fluchtzeit durch N | Eine höhere Rechentiefe verändert auch schon bekannte Fluchtwerte. Mapping-Skalierung von N trennen. |
| X/Y-Auflösung 0.001, Float-Parameter | Keine brauchbare Tiefzoom-Navigation. Double-Zentrum plus relative Navigation und begrenzten Zoom entwerfen. |
| MIDI-Clamp nach Skalenmapping | Bei 0/127 kann die Skalenbindung verloren gehen. Aus erlaubter Skalennotenliste im Tonbereich auswählen. |
| Mode wirkt blockweise | Kein musikalisch quantisierter GEN/CTRL-Wechsel. Wechsel am nächsten Sequenzzyklus übernehmen. |
| Parameteraufnahme hängt von Blockgröße ab | Die geprüfte Blockunabhängigkeit gilt bei konstanten Parametern; für Automation ist sie nicht allgemein bewiesen. |
| Weder Glättung noch Änderungslimit für Pitch | Kleine Koordinatenänderungen können große Sprünge verursachen. Begrenzter Tonbereich, optional musikalisches Step-Intervalllimit. |
| Tests prüfen MIDI-Grenzen hauptsächlich am Standardpunkt | Kein umfassender Parameterrand-/Skalentreue-Test, kein APVTS-Roundtrip-Test und kein eigener Moduswechseltest. Ergänzen. |
| Berechnung im Audio-Thread | 96 Iterationen pro ausgelöstem Step sind begrenzt; tiefere Analyse gehört in einen Worker. Kein Leistungsnachweis für beliebige Iterationstiefen. |

Eine unabhängige Python-Analyse ergibt beim Standardpreset X=−0.6,Y=0,A=0.5, Radius=0.37: zwölf ungelöste Punkte bei N=96, zwei Fluchten bei n=11, zwei bei n=18. Bei Root 48/Dur ergibt GEN somit zwölfmal MIDI 95 und je zweimal MIDI 53 bzw. 57. Das ist eine Berechnung, kein Hörtest und kein neuer C++-Testlauf.

Ein weiteres Beispiel: c=−0.75+0.001i flieht in dieser Rechnung erst bei n=3143. Bei N=96 oder 512 wäre es als „nicht geflohen“ erfasst. Numerische Ergebnisse und Methodik stehen in Mandelbrot-v02-numerical-analysis.json.

## 3. Mathematische Grundlage und Aussagegrenzen

M ist die Menge der Parameter c, für die die kritische Bahn von 0 beschränkt bleibt. Navigation verändert den Parameter c; sie ist keine Variation des Anfangswerts z_0. Flucht ist ein positiver Befund, fehlende Flucht bei endlichem Budget meist nur ein unentschiedener Befund. Nicht jede beschränkte Bahn konvergiert auf einen anziehenden Zyklus. [Cornell: The Mandelbrot Set](https://pi.math.cornell.edu/~lipa/mec/lesson5.html).

Selbstähnlichkeit ist kein Versprechen gleicher 16-Step-Melodien auf beliebigen Zoomstufen. Mathematische Aussagen gelten unter bestimmten Voraussetzungen, etwa asymptotisch an Misiurewicz-Punkten. Unsere endliche Abtastung und Skalenquantisierung können solche Beziehungen auch verdecken. [Dudko/Grigorieva: Surgery sequences and self-similarity](https://arxiv.org/abs/2202.09832).

## 4. Navigation und zwei unterschiedliche Untersuchungen

**Contour:** Jeder Step untersucht einen anderen Parameter c_s. Das hört räumliche Unterschiede zwischen 16 kritischen Bahnen.

**Orbit:** Alle Steps untersuchen dieselbe Bahn zu einem einzigen c=X+iY, nach einem expliziten Vorlauf B. Das hört zeitliche Iterationsstruktur. Iterationsnummer wird bewusst auf musikalische Zeit übertragen; sie besitzt selbst keine BPM.

Contour ist der Vorschlag für den ersten kleinen Funktionsumfang. Orbit ist das wichtigste nachfolgende Forschungsexperiment, weil periodische Bahnstruktur dort direkt als Folge sichtbar und hörbar bleiben kann. Diese zwei Sichtweisen dürfen nicht verwechselt werden.

Für Contour: c_s=C+(r/Z)u_s. Z≥1 ist Zoom; r ist Radius im normierten lokalen Abtastfenster; r/Z ist die tatsächliche Koordinatenausdehnung und wird angezeigt. Zoom und Radius sind getrennte Bedienelemente, beeinflussen bei dieser Definition aber gemeinsam die Welt-Ausdehnung. Diese Redundanz ist ausdrücklich dokumentiert. Falls Radius in Weltkoordinaten definiert werden soll, wäre Zoom ausschließlich die Ansichtsvergrößerung; beide Definitionen nicht mischen.

- Kreis: u_s=exp(2πis/16).
- Linie: u_s=−1+2s/15; zunächst horizontal, spätere Rotation explizit.
- Raster als späteres Experiment: j=s mod 4,k=floor(s/4), u_s=((2j/3−1)+i(2k/3−1))/√2. Reihenfolge zeilenweise; Reihenwechsel sind keine lokalen Nachbarschaften und dürfen nicht als solche analysiert werden.
- Freie Bahn später: gespeicherte Kontrollpunkte und definierte Interpolation/Abtastung nach Bogenlänge. Keine implizite Zufallsbewegung.

Vorschlag: N wählbar 64/128/256/512, Double-Rechnung, Zoom zunächst höchstens 10^4. Keinen Deep-Zoom-Anspruch erheben. Hoher Zoom macht das endliche Budget nicht automatisch ausreichend; Positionen mit identischen Double-Samples werden als numerisch unaufgelöst gekennzeichnet. Eingaben zusätzlich auf NaN/Inf und Überläufe prüfen.

## 5. Kandidaten für mathematische Merkmale

### A. Geglättete Fluchtzeit μ — für Außenstrukturen

**Eigenschaft/Berechnung:** Nach erster Flucht bei n zwei weitere sichere Iterationen, m=n+2, dann μ=m+1−ln(ln|z_m|)/ln2. Bei nicht endlichen Zwischenwerten Ergebnis ungültig, kein NaN-Mapping. Für fehlende Flucht existiert kein so berechneter μ-Wert.

**Interpretation:** Näherung einer renormalisierten Fluchtzählung im Außenbereich, kein exakter Abstand zur Menge und keine überall perfekte Stetigkeit. Zusätzliche Iterationen verringern endliche Cutoff-Artefakte. [Linas Vepstas: Renormalizing the Mandelbrot Escape](https://www.linas.org/art-gallery/escape/escape.html).

**Mapping:** f=clamp(log(1+max(μ,0))/log(1+L),0,1), zunächst L=64 als gespeicherte musikalische Referenz. N beeinflusst die Verfügbarkeit, nicht diesen Maßstab. Für ungelöste Punkte darf die Option f=1 nur als musikalische Plateau-Konvention verwendet werden; Status bleibt getrennt.

**Wirkung/Grenzen/Hörtest:** Feinere Außenkonturen als Integer-Bänder, aber Skalenquantisierung erzeugt weiterhin Tonstufen. Außenposition −0.75+0.1i langsam entlang Y verändern; mit Integer-Escape bei gleichem Tonbereich vergleichen. μ löst das Innenraumproblem nicht.

### B. Mittlerer begrenzter Bahnbetrag E — auch für ungelöste Punkte

**Eigenschaft/Berechnung:** K=min(N,erster Fluchtindex), bei fehlender Flucht K=N. E=(1/K)Σ_(n=1)^K min(|z_n|²/4,1).

**Interpretation:** Ein exakt definierter numerischer Deskriptor des endlichen Bahnpräfixes. Die Begrenzung verhindert Dominanz eines letzten großen Fluchtwerts. Keine Fraktaldimension, kein Maß für Chaos und keine physikalische Energie; im UI „Orbit Magnitude“ nennen.

**Mapping:** f=E für Pitch oder Velocity. E∈[0,1]. N/K, Begrenzung und Start bei z_1 bleiben feste, dokumentierte Konventionen. Unterschiedlich lange Präfixe vergleichen unterschiedliche Beobachtungsfenster; das ist eine Grenze, kein asymptotisches Invariant.

**Wirkung/Grenzen/Hörtest:** Innenbahnen werden nicht pauschal gleich. Bei geradem N und Radius 0: c=0 liefert E=0, c=−1 liefert E=1/8, c=−2 liefert E=1. Mit Orbit Magnitude als Velocity-Quelle und festem Pitch anhören. Ob der Unterschied musikalisch nützlich ist, muss gehört werden.

### C. Mittlere Bahnbewegung V — dynamische Akzente

**Eigenschaft/Berechnung:** V=(1/K)Σ_(n=1)^K min(|z_n−z_(n−1)|/4,1), gleicher K wie B.

**Interpretation:** Bewegung zwischen aufeinanderfolgenden Orbitpunkten im endlichen Präfix. Keine Lyapunov-Exponente. Starttransient und variable Präfixlänge wirken mit.

**Mapping:** V als unabhängige Velocity-Quelle. **Erwartung:** Bahnen mit größeren mittleren Bewegungen werden stärker akzentuiert. **Hörtest:** c=0 gegen c=−1, alle Tonhöhen gleich. Für −1 beträgt V=1/4; der Zweierzyklus bewegt sich, der Null-Fixpunkt nicht. Das ist keine Behauptung „Periodizität = Lautstärke“.

### D. Räumlicher Nachbarkontrast H — Rhythmus und Akzente

**Eigenschaft/Berechnung:** Auf dem Kreis H_s=|E_s−E_(s−1 mod 16)|. Auf der offenen Linie H_0=0 und H_s=|E_s−E_(s−1)| für s>0. h_s=min(H_s/κ,1), zunächst κ=0.05, gespeichert und im Mapping dokumentiert.

**Interpretation:** Endliche räumliche Differenz des Orbitmerkmals, keine zeitliche chaotische Sensitivität und keine Ableitung. Sampling-Abstand und Reihenfolge wirken mit. Für eine echte finite Differenzenquote zusätzlich durch |c_s−c_(s−1)| teilen; für den ersten Rhythmusversuch bewusst nicht, um Tiefzoom-Rauschen nicht zu verstärken.

**Mapping:** Hoher h_s schützt eine Note vor rhythmischer Ausdünnung oder erzeugt einen Akzent. **Grenzen:** Sehr homogene Gebiete können bei starkem Rhythm Amount ganz stumm werden; das ist ein legitimes Ergebnis. Keine automatisch eingefügten „Fraktal-Kicks“. **Hörtest:** gleicher Kreis, feste Note/Velocity; Region verändern und Pausen mit der angezeigten H-Folge vergleichen. Kreis gegen Linie separat testen.

### E. Periodenkandidaten und Argument — später, nicht im ersten Slice

Für ungelöste Bahnen: nach B Iterationen für p=1…16 über W=32 Vergleiche max_j |z_j−z_(j−p)|/(1+|z_j|)<ε, etwa ε=10^−8. Kleinsten erfolgreichen p als Kandidaten ausgeben. Kein Beweis für echte Periodizität oder Zugehörigkeit. Nahe parabolischen Punkten, langen Transienten und endlicher Präzision sind Fehlklassifikationen möglich. Für einen Attraktionsbefund zusätzlich den Zyklusmultiplikator ∏2z_j untersuchen; eine numerische Schätzung bleibt kein zertifizierter Beweis.

Orbit-Modus: q_s=min(|z_(B+s+1)|/2,1) als Pitch-/Velocity-Quelle. Vorzeitige Flucht markiert ungültige spätere Orbitsteps; nicht auf einen erfundenen Dauerzyklus auffüllen. Beispiel B=0,c=−1 liefert abwechselnd Beträge 1 und 0. Abbildung auf Sechzehntel macht eine mathematische Zweierfolge hörbar. c=−2 ist präperiodisch, kein anziehender Zweierzyklus.

Argument arg(z) ist bei z=0 undefiniert, an der ±π-Grenze sprunghaft und für reelle Bahnen oft nur 0/π. Wenn untersucht, zunächst sin(arg z),cos(arg z) mit Gültigkeitsflag, keine rohe Winkelquantisierung als Standard. Periodenkandidat→musikalische Länge und Winkel→Tonhöhe wären zusätzliche Designregeln. Noch keine Implementierungsempfehlung dafür.

## 6. Konkrete unabhängige MIDI-Mappings

Alle Regeln sind musikalische Interpretationen der obigen messbaren Merkmale. Quellenwahl und Amount sind pro Engine unabhängig. Die Navigation ist gemeinsam und darf daher mehrere Features gleichzeitig verändern.

### Pitch

Erzeuge die sortierte Liste S aller zur Skala/Root gehörenden MIDI-Noten innerhalb Low…High. Ein leerer Bereich ist ein Validierungsfehler, kein stiller Skalenwechsel. P=len(S), f_s∈[0,1]. Zielindex t_s=round((P−1)f_s).

GEN: Basisindex b ist der gültige Root-Index oder die nächstgelegene erlaubte Note. CTRL: b_s entspricht der gespeicherten Basissequenz. i_s=clamp(round(b_s+A_pitch(t_s−b_s)),0,P−1), Ausgabe S[i_s]. Bei A_pitch=0 bleibt CTRL exakt erhalten, sofern das gespeicherte Pattern im gültigen Tonbereich liegt. Für außerhalb liegende Basistöne muss die Produktspezifikation entscheiden: unveränderte Wiedergabe bei Amount=0 mit Warnung oder vorab bestätigte Pattern-Quantisierung. Keine heimliche Korrektur des Originalpatterns.

Alternative für CTRL: additive Variation round(A_pitch·D·(2f_s−1)), D als maximaler Stufenhub; besser für motivische Identität, aber musikalisch weniger direkte Angleichung an die Kontur. Empfehlung: additive Variation für CTRL, absolutes Ziel für GEN; beides im UI benennen. Low/High und Stufenhub vermeiden unkontrollierte Registersprünge.

### Rhythm

Basismaske b_s ist in CTRL das gespeicherte Pattern, in GEN die manuelle Step-On-Maske. on_s=b_s AND (h_s≥A_rhythm·T), T zunächst 0.5 als gespeicherter Schwellenwert.

A_rhythm=0 erhält jede Basismaske exakt. Höhere Werte entfernen zuerst kontrastarme Steps. Basispause bleibt Pause, es werden im ersten Slice keine neuen Noten an zuvor ausgeschalteten Steps erzeugt. Zurückdrehen stellt das Original wieder her. T ist eine Kontrastschwelle, kein Versprechen einer exakten Notendichte. Kein Zufall, keine versteckte Schrittpriorität. Im homogenen Gebiet kann alles wegfallen; Rhythm Amount=0 ist der sichere Rückweg.

Alternative: die stärksten K Kontraste auswählen. Das kontrolliert Notendichte, braucht aber eine explizite Gleichstandsregel, die bei identischen Merkmalen künstliche Ordnung schafft. Deshalb erst im Vergleichstest untersuchen.

### Velocity

CTRL erhält neue, gespeicherte Basisvelocity pro Step v_s; GEN verwendet zunächst die globale Basis. Ausgabe clamp(v_s+round(A_velocity·D_v·(2g_s−1)),1,127), g_s wahlweise E_s,V_s oder h_s, D_v zunächst 30. A_velocity=0 erhält die Basis exakt. Die Velocity-Engine darf das Originalpattern nicht überschreiben.

Empfohlene Exploration: Pitch aus E oder μ, Rhythm aus H, Velocity aus V. Sie verwenden unterschiedliche Eigenschaften; Unabhängigkeit der Regler bedeutet aber keine statistische Unabhängigkeit dieser Merkmale.

## 7. Architektur und Echtzeitverhalten

NavigationSnapshot → Sampler → OrbitAnalyzer → FeatureFrame → drei Mapper → PatternFrame → HostScheduler → MIDI.

- NavigationSnapshot speichert Zentrum/Zoom/Radius/Sampler/N als zusammengehörigen Zustand.
- FeatureFrame enthält pro Step Escape-Flag/-Index, gültiges μ, E,V,H und Analyseversion. „Unresolved“ bleibt eigenständig.
- Worker berechnet bei Änderungen begrenzte 16-Punkt-Frames. Keine Allokationen, Mutex-Waits oder tiefe Fraktalberechnung im Audio-Callback. Übergabe über vorallokierte SPSC-Struktur mit klaren Besitzregeln, kein unsicheres Doppelbuffer-Überschreiben.
- Am Beginn eines Vier-Viertel-Zyklus übernimmt der Scheduler den neuesten vollständig fertigen Frame und den angeforderten GEN/CTRL-Modus. Basispattern und hörbares abgeleitetes Pattern getrennt halten.
- Bei veraltetem Frame bleibt die letzte vollständige Sequenz aktiv, mit Statusanzeige. Asynchrone Fertigstellung kann jedoch den Übernahmezyklus verändern. Für exakt reproduzierbare Automation/Offline-Render ist ein separater synchroner, begrenzter Berechnungsweg mit festen Übernahmezeitpunkten erforderlich. Dies ist ein Architekturprüfpunkt vor Implementierung.
- Run/Panic/Stop wirken ohne Warten auf die Analyse; Gate/aktive Note behalten sichere Note-Off-Verwaltung. Keine mathematische Quelle direkt als unbeschränkte Notenlänge verwenden.
- Optionales Pitch-Intervalllimit verändert bewusst das Sonifikationsergebnis und muss abschaltbar sein. Positionsglättung ist eine zeitliche Navigation, nicht eine Änderung der Mandelbrot-Formel. Für den ersten Slice quantisierte Übernahme statt versteckter Glättung.

## 8. Presets, Rückfall und APC20

v0.1-Binärdatei und Quellstand unverändert archivieren; v0.2 als eigenes Plug-in mit eigenem VST3-Identifier entwickeln, damit beide in Live nebeneinander geladen werden können. Das ist ein Vorschlag, noch keine Repository- oder Binäränderung.

Schema-versionierte Presets speichern komplette Navigation, Feature-/Mappingwahl, Amounts, Root/Scale/Tonbereich, Basismaske/-Degrees/-Velocities, Gate und Positionsslots. Position-only-Slots verändern kein Basismuster. Neue Zustände validieren und atomar übernehmen. Alte v0.1-Presets entweder explizit in einem Legacy-Profil unverändert wiedergeben oder kontrolliert konvertieren; eine Konvertierung ohne Legacy-Mapping kann keine identische Musik versprechen.

APC20: acht Fader für relative X-/Y-Navigation, Zoom, Radius, Pitch Amount, Rhythm Amount, Velocity Amount, Gate; Tasten für quantisierten Mode, Run/Panic und Positionsspeicher. Relative Navigation verwendet ein gespeichertes Double-Zentrum plus Faderoffset im aktuellen Zoomfenster. Ein absolutes X/Y-Faderpaar über den ganzen Bereich reicht für feine Zoomnavigation nicht aus.

Positionsslots zunächst im einfachen Plug-in-Editor speichern/abrufen, später über hostautomatisierbare Trigger. Schreiben nur auf definierter steigender Flanke; kein ständiges Speichern bei gehaltenem Taster. Bestehende APC20-Scripts unverändert lassen. Ein späterer Mappingtest muss Konflikte mit bereits belegten Tasten feststellen; keine pauschale Deaktivierung als Entwicklungsschritt.

## 9. Kleiner vorgeschlagener Implementierungsumfang nach Freigabe

v0.2a: vorhandenes Zwei-Spur-Routing und Note-Off-Scheduler erhalten; Kreis plus Linie; getrennte Navigation und drei Amounts; E,V,H plus Integer-/Smooth-Escape zum Vergleich; begrenzter Tonbereich; CTRL-Originalpattern einschließlich Step-Velocities; quantisierter Modewechsel; klare Gültigkeits-/Unresolved-Anzeige; Zustandsspeicherung.

Noch zurückstellen: frei gezeichnete Bahn, Raster, Periodensteuerung, hierarchische Sequenzen, selbstständige Exploration, Zufallsvariationen, Anspruch auf mathematisch begründete motivische Selbstähnlichkeit und aufwendiges GUI. Positionsslots können als zweite kleine Ausbaustufe folgen. Ein kleiner Merkmals-/Step-Wertebereich im Editor ist zur Nachvollziehbarkeit sinnvoller als eine dekorative Fraktalgrafik.

## 10. Validierung und Hörversuche

Automatisch nach Implementierung: exakte einfache Orbits c=0,−1,−2,1; Escape-Konvention; finite-budget-Unresolved; Double-Konjugationssymmetrie für Betragsmerkmale; Betrag-/Bewegungsgrenzen; NaN/Inf; Scale/Low/High-Grenzen; identische Events über Blockgrößen; negative PPQ, Stop/Seek/Loop, Panic und Sample-Rate-Wechsel; Modewechsel ausschließlich an Zyklusgrenzen; unabhängige Amounts; CTRL-Original bei allen Amounts=0; Preset-/Positionsslot-Roundtrip mit identischen MIDI-Ereignissen; Legacy-Kompatibilität. Asynchroner Frame-Lifetime-Stresstest und Vergleich gegen synchronen Renderpfad.

Hörtest A: E bei c=0/−1/−2, Radius=0, zunächst konstante Note und nur Velocity. Hörtest B: μ gegen Integer-Escape auf derselben Außenbahn, feste Normalisierung. Hörtest C: Rhythm aus H bei konstantem Pitch/Velocity gegen dieselbe Notenzahl mit gleichmäßig verteilten Steps. Hörtest D: dieselben Featurewerte in ursprünglicher gegen deterministisch permutierter Reihenfolge; Wertverteilung bleibt erhalten, räumliche Ordnung wird zerstört. Hörtest E später: Orbit-Modus c=−1 gegen eine explizit entworfene Zweiersequenz, um zu prüfen, ob Mathematik mehr bietet als dieselbe musikalische Struktur von Hand.

Für zufallsbasierte Referenzen separaten Offline-Vergleich mit gespeichertem Seed verwenden, nie still im Fraktalpfad. Gleicher Synth, gleiches Tempo, Register und annähernd gleiche Notendichte/Velocityverteilung; Varianten in wechselnder Reihenfolge hören. Beobachtung protokollieren: Wiedererkennbarkeit, Reaktion auf Koordinatenänderung, Steuerbarkeit und musikalischer Nutzen. Es liegen noch keine solchen Hörtest-Ergebnisse vor.

CPU-Profilierung später auf dem Zielrechner bei 48/96 kHz und 64/512/2048 Samples, schneller APC-Navigation und maximal freigegebenem N. Eine erfolgreiche CI-Kompilierung ersetzt diesen Test nicht.

## Freigabepunkte

1. Contour als erster Slice, Orbit als nächstes Experiment?
2. Pitch in CTRL additiv variieren, GEN absolut auf begrenzte Skalenliste abbilden?
3. Rhythm zunächst nur kontrastarme Basissteps entfernen, keine neuen Steps hinzufügen?
4. Neues Plug-in neben v0.1 und zyklusweise Übernahme als Performance-Standard?

Bis zur gemeinsamen Prüfung bleibt dies ein Konzept. Die v0.1-Implementierung wurde für diesen Auftrag nicht verändert.
