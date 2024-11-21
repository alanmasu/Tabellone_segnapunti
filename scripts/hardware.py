import os

# Percorso del file da verificare
file_path = "include/hardware.h"

# Contenuto da inserire nel file se deve essere creato
file_content = """#ifndef __HARDWARE_H__
#define __HARDWARE_H__

// #ifndef PULSANTEIRA_MAC_ADDRESS 
//     #define PULSANTEIRA_MAC_ADDRESS {, , , , , };
// #endif

// #ifndef TABELLONE_MAC_ADDRESS 
//     #define TABELLONE_MAC_ADDRESS {, , , , , };
// #endif

#endif
"""

def check_and_create_file(path, content):
    
    # Controlla se il file esiste
    if os.path.exists(path):
        print(f"Il file '{path}' esiste già.")
    else:
        # Ottieni la directory dal percorso del file
        directory = os.path.dirname(path)
        
        # Crea la directory se non esiste
        if not os.path.exists(directory):
            os.makedirs(directory)
            print(f"Creata la directory: {directory}")
        
        # Crea il file con il contenuto specificato
        with open(path, 'w') as file:
            file.write(content)
        print(f"Creato il file: {path} con il contenuto specificato.")

# Esegui la funzione
check_and_create_file(file_path, file_content)
