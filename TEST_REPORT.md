# v0.3 Prüfbericht

Status: Phase-3-Prototyp implementiert; neuer Windows-Build und Tests noch ausstehend. v0.2b-Ergebnisse gelten nicht als v0.3-Tests.

Zwischenergebnisse: GitHub-Lauf 38168217311 scheiterte am Import vorhandener Ordner; Merge mit Copy-Item -Force lokal geprüft. Lauf 38168348928 kompilierte erfolgreich. Engine-, GUI-, ViewMath- und Evolution-Tests bestanden, Processor-Test scheiterte am exakten Roundtrip der mathematischen Abtastpunkte. Verlustfreie Double-Speicherung implementiert; der Test bleibt exakt. Vollständiger neuer Build erforderlich. Der neue Processor-Test setzt zusätzlich reale JUCE-nonRealtime-Flags und prüft gespeicherte Ranking-Merkmale/Nav-Werte. Evolution-Test prüft zusätzlich Flight Stop ohne Positionssprung.

Geplant: Engine-, Processor-, GUI-, ViewMath- und EvolutionTests. Evolution prüft A (32-Takt-Hold), B (Acht-Takt-Grenzen), C (maximal ein Step), D (wiederholte MIDI-Folgen), F (Freeze), H (Note-On/Off-Bilanz), I (unterschiedliche Blockgrößen im Engine-Harness), LOW und feste Gleichstandsregel. Processor-Test enthält Capture/Preset mit Noten und mathematischen Punkten. E und G bleiben wegen nicht implementierter Struktur-/VISUAL-Modi ausdrücklich ausstehend. Offline-Test ist eine Engine-Simulation, kein Ableton-Offline-Export.

Ableton-/APC20-Prüfung, visuelle Flugabnahme, CPU-Messung und Host-Automation stehen aus. Grenzen: TECHNICAL_CONCEPT.md.
