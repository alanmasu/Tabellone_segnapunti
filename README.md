# Tabellone Segna punti

## Introduzione
Progetto nato all'ITST J.F. Kennedy di Pordenone, con l'obbiettivo di studiare tecniche e metodologie per la realizzazione di un tabellone segnapunti per la palestra della scuola. Il progetto è stato realizzato da un gruppo di studenti del quarto anno del corso di Elettrotecnica, con l'ausilio di due docenti.

## Descrizione
Il progetto è basato su tecnologia a micro controllore e come MCU è stata scelta l'ESP32 di Espressif.

Si compone di due parti:
- Tabellone segnapunti
- Pulsantiera

Le due componenti comunicano, nella versione attuale, trammite WiFi, ma è prevista una versione con comunicazione tramite il protocollo proprietario ESP-NOW.

## Tabellone segnapunti
Il tabellone segnapunti è composto da vari display a 7 segmenti, da un modulo WiFi e da un modulo RTC. 
Il tabellone ha due modalità di funzionamento: Tabellone e Orologio.

### Mod. Tabellone

Il tebellone, in questa modalità, è in grado di visualizzare i seguenti dati:
- Punteggi delle due squadre
- Conto alla rovescia di gioco, impostabile da 1 a 99 minuti
- Periodo di gioco
- Falli di squadra
- Timeout di squadra

Tutti questi dati si possono impostare facilmente dalla pulsantiera.

### Mod. Orologio
In questa modalità viene visualizzata l'ora nei display relativi al cronometro. Viene tenuta aggiornata tramite il modulo RTC (DS3231).

Questa modalità è stata pensata per essere utilizzata quando il tabellone non è in uso, ed entra in funzione automaticamente quando viene rilevata una disconnessione dalla pulsantiera che perdura da più di 30 secondi; è possibile entrare in questa modalità anche manualmente con una combinazione di tasti da pulsantiera.

## Pulsantiera
E' composta da un modulo WiFi e da 17 pulsanti, ognuno con una funzione specifica.

### Pulsanti


| Pulsante | Funzione | Funzione [Shift] | Funzione Mod. Orologio + [Shift] |Note|
|:--------:|:--------:|:----------------:|:---------------------:|:--:|
| Start | Start | Entra in mod. Orologio | - | - |
| Stop | Stop | Esce da mod. Orologio | - | - |
| Reset | Azzera tutto il tabellone | - | - | Attivo solo se il cronometro/timer è fermo |
| Punti A + | Aggiunge 1 punto alla squadra A | Aggiunge 1 fallo alla squadra A | - | Con pressione prolungata +5 Punti|
| Punti A - | Sottrae 1 punto alla squadra A | Sottrae 1 fallo alla squadra A | - | Con pressione prolungata -5 Punti|
| Punti B + | Aggiunge 1 punto alla squadra B | Aggiunge 1 fallo alla squadra B | - | Con pressione prolungata +5 Punti|
| Punti B - | Sottrae 1 punto alla squadra B | Sottrae 1 fallo alla squadra B | - | Con pressione prolungata -5 Punti|
| Punti Reset | Azzera i punti delle due squadre | Azzera i falli delle due squadre | - | Attivo solo se il cronometro/timer è fermo |
| Minuti + | Aggiunge 1 minuto al conto alla rovescia di gioco | Aggiunge 1 timeout alla squadra A | Incrementa di 1 le ORE impostate| Con pressione prolungata +5 Minuti/Ore |
| Minuti - | Sottrae 1 minuto al conto alla rovescia di gioco | Sottrae 1 timeout alla squadra A | Decrementa di 1 le ORE impostate| Con pressione prolungata -5 Minuti/Ore |
| Secondi + | Aggiunge 1 secondo al conto alla rovescia di gioco | Aggiunge 1 timeout alla squadra B | Incrementa di 1 i MINUTI impostati| Con pressione prolungata +5 Secondi/Minuti |
| Secondi - | Sottrae 1 secondo al conto alla rovescia di gioco | Sottrae 1 timeout alla squadra B | Decrementa di 1 i MINUTI impostati| Con pressione prolungata -5 Secondi/Minuti |
| Tempo Reset | Azzera il conto alla rovescia di gioco | Azzera i timeout delle due squadre | - | Attivo solo se il cronometro/timer è fermo |
| Periodo + | Aggiunge 1 periodo al conto alla rovescia di gioco | - | - | - |
| Periodo - | Sottrae 1 periodo al conto alla rovescia di gioco | - | - | - |
| Periodo Reset | Azzera il periodo di gioco | - | - | Attivo solo se il cronometro/timer è fermo |

## ChangeLog
  - 18/02/2020: Suddiviso il codice in funzioni
  - 22/05/2020: Aggiunto il powerfail detector nel tabellone
  - 25/06/2020: Spostato il modulo RTC sul tabellone
  - 25/06/2020: Integrato l'avanzamento dei minuti e dei secondi con pressione prolungata
  - 27/05/2020: Aggiunto l'RTC
  - 27/05/2020: Modificata la lettura dei pulsanti virtuali
  - 27/05/2020: Aggiunto l'avanzamento veloce dei punti dovuti a pressione prolungata
  - 20/10/2023: [BUGFIX] Cambiati gli indirizzi IP secondo specifiche Espressif
  
## Dipendenze
  - ESP32 Arduino Core (1.0.4) by [Espressif](https://github.com/espressif/arduino-esp32/tree/1.0.4)
  - Adafruit MCP23017 (1.3.0) by [Adafruit](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library/tree/1.0.3)
  - SetteSeg by Alan Masutti