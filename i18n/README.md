# Translation System mit Hash-Support und Timestamp-Tracking

## Übersicht

Das Übersetzungssystem unterstützt jetzt:
- **Kurze Strings** (< 80 Zeichen): Direkter Key mit Timestamp
- **Lange Strings** (≥ 80 Zeichen): CRC32-Hash als Key mit Timestamp
- **Automatische Timestamp-Updates**: Im Debug-Modus wird `lastSeen` bei jedem Zugriff aktualisiert
- **Cleanup-Tool**: Entfernt ungenutzte Übersetzungen basierend auf Timestamp

## Dateiformat

### Alte Format (weiterhin unterstützt)
```ini
[Translation]
id=Button
Add=Neu
Remove=Löschen
```

### Neues Format

**Kurze Strings:**
```ini
[Translation]
id=Button
{s:"Add",t:"2025-12-09"}=Neu
{s:"Remove",t:"2025-12-09"}=Löschen
```

**Lange Strings:**
```ini
[Translation]
id=Snackbar
{h:"a3f5b2c1",s:"Welcome to the Snackbar System!\n\nSnackbars di",t:"2025-12-09"}=Willkommen beim Snackbar-System!\n\nSnackbars zeigen...
```

### Format-Spezifikation

- `s:` - String (vollständig bei kurzen Strings, gekürzt auf 50 Zeichen bei langen)
- `h:` - CRC32-Hash (8 Hex-Zeichen) für lange Strings
- `t:` - Timestamp im Format YYYY-MM-DD (lastSeen)

## Funktionsweise

### Im Debug-Modus (QAPLA_DEBUG_I18N)

1. **Fehlende Übersetzungen**: Automatisch zu allen `.lang`-Dateien hinzugefügt
2. **Timestamp-Updates**: Bei jedem `translate()`-Aufruf wird der Timestamp aktualisiert
3. **Neues Format**: Neue Einträge werden im neuen Format mit Timestamp erstellt

### Im Release-Modus

- Verwendet eingebettete Übersetzungen (kompiliert in die .exe)
- Fehlende Keys werden in `missing_translations.txt` geloggt
- Keine Timestamp-Updates (Performance)

## Cleanup-Tool

### Verwendung

```powershell
# Dry-Run: Zeigt was entfernt würde, ohne Änderungen
.\cleanup_old_translations.ps1 -DaysOld 90 -DryRun

# Tatsächliches Cleanup: Entfernt Einträge älter als 90 Tage
.\cleanup_old_translations.ps1 -DaysOld 90

# Kürzere Zeitspanne (z.B. 30 Tage)
.\cleanup_old_translations.ps1 -DaysOld 30
```

### Workflow-Empfehlung

1. **Entwicklung**: Debug-Build verwenden
   - Timestamps werden automatisch aktualisiert
   - Neue Übersetzungen werden automatisch hinzugefügt

2. **Regelmäßiges Testing**: Alle Features durchklicken
   - Stellt sicher, dass verwendete Strings aktuelle Timestamps haben

3. **Cleanup**: Monatlich oder vor Release
   ```powershell
   # Erst Dry-Run zur Überprüfung
   .\cleanup_old_translations.ps1 -DaysOld 90 -DryRun
   
   # Dann tatsächliches Cleanup
   .\cleanup_old_translations.ps1 -DaysOld 90
   ```

4. **Backup**: Tool erstellt automatisch `.bak`-Dateien

## Technische Details

### CRC32-Hash

- **Algorithmus**: Standard CRC32 (polynomial 0xEDB88320)
- **Kollisionswahrscheinlichkeit**: ~0.01% bei 1000 Strings
- **Format**: 8 Hex-Zeichen (z.B. "a3f5b2c1")

### String-Normalisierung

Vor der Übersetzung werden Strings normalisiert:
- Zahlen → Platzhalter `\1`, `\2`, etc.
- `=` Zeichen → Platzhalter
- Whitespace/ImGui-IDs entfernt

Beispiel:
```cpp
translate("Message", "We played 5/10 games")
// Normalisiert zu: "We played \1/\2 games"
```

### Abwärtskompatibilität

Das System unterstützt beide Formate gleichzeitig:
- Alte Einträge: `Key=Value`
- Neue Einträge: `{s:"Key",t:"..."}=Value`

Beim Laden werden beide erkannt und korrekt verarbeitet.

## Migration von alten Dateien

Bestehende `.lang`-Dateien müssen nicht konvertiert werden:
1. Alte Einträge funktionieren weiterhin
2. Neue Einträge werden automatisch im neuen Format hinzugefügt
3. Beim nächsten Cleanup bleiben alte Einträge erhalten (kein Timestamp)

Optional: Manuelle Konvertierung mit Tool möglich (zukünftige Erweiterung).

## Beispiel-Workflow

```cpp
// Im Code:
auto text = tr("Button", "Add");

// Im Debug-Build:
// 1. Prüfe ob Übersetzung existiert
// 2. Wenn ja: Update Timestamp auf heute
// 3. Wenn nein: Füge hinzu mit aktuellem Timestamp

// In deu.lang (neu hinzugefügt):
{s:"Add",t:"2025-12-09"}=Neu

// Nach 3 Monaten ohne Nutzung (kein Zugriff auf diesen Button):
// cleanup_old_translations.ps1 -DaysOld 90
// → Eintrag wird entfernt (lastSeen > 90 Tage alt)
```

## Best Practices

1. **Debug-Build nutzen**: Während der Entwicklung immer Debug-Build verwenden
2. **Features testen**: Vor Cleanup alle Features einmal durchklicken
3. **Vorsichtig cleanen**: Lieber konservative Zeitspanne wählen (90+ Tage)
4. **Backups prüfen**: `.bak`-Dateien werden automatisch erstellt
5. **Review vor Cleanup**: `-DryRun` verwenden zur Kontrolle

## Limitierungen

- **Hash-Kollisionen**: Theoretisch möglich bei ~4000+ Strings (sehr unwahrscheinlich)
- **Manuelle Edits**: Timestamps in manuell editierten Einträgen müssen manuell aktualisiert werden
- **Format-Strings**: `std::format()` mit dynamischen Werten können schwer zu tracken sein
