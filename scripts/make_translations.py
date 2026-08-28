#!/usr/bin/env python3
"""Writes the .ts catalogues from one table.

lupdate is not available in any lane that runs here, and hand-editing seven
XML files per string change is how catalogues drift. So the translations live
in one dictionary keyed by the English source, the contexts are read back out
of the QML, and the files are generated.

Keying by source rather than by (context, source) is deliberate: "Settings"
appears in three files and should not be able to come out differently in each
of them.

Run it after adding or changing a qsTr(), then commit the result:

    docker compose run --rm checks     # tells you what is missing
    python3 scripts/make_translations.py
"""

import re
import sys
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parent.parent
QML_DIR = ROOT / "qml"
TS_DIR = ROOT / "translations"

LOCALES = ["en", "fr", "de", "es", "fi", "it", "nb_NO"]

QSTR = re.compile(r'qsTr\(\s*"((?:[^"\\]|\\.)*)"')
BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
LINE_COMMENT = re.compile(r"//[^\n]*")

# Plural entries are a [singular, plural] pair; every language here uses two
# numerus forms, which is what keeps this table readable.
TRANSLATIONS = {
    # --- actions and short labels -------------------------------------
    "About": {
        "en": "About", "fr": "À propos", "de": "Über", "es": "Acerca de",
        "fi": "Tietoja", "it": "Informazioni", "nb_NO": "Om",
    },
    "Settings": {
        "en": "Settings", "fr": "Réglages", "de": "Einstellungen",
        "es": "Ajustes", "fi": "Asetukset", "it": "Impostazioni",
        "nb_NO": "Innstillinger",
    },
    "History": {
        "en": "History", "fr": "Historique", "de": "Verlauf",
        "es": "Historial", "fi": "Historia", "it": "Cronologia",
        "nb_NO": "Historikk",
    },
    "Done": {
        "en": "Done", "fr": "Terminé", "de": "Fertig", "es": "Listo",
        "fi": "Valmis", "it": "Fatto", "nb_NO": "Ferdig",
    },
    "Stop": {
        "en": "Stop", "fr": "Arrêter", "de": "Anhalten", "es": "Detener",
        "fi": "Pysäytä", "it": "Ferma", "nb_NO": "Stopp",
    },
    "Open": {
        "en": "Open", "fr": "Ouvrir", "de": "Öffnen", "es": "Abrir",
        "fi": "Avaa", "it": "Apri", "nb_NO": "Åpne",
    },
    "Open folder": {
        "en": "Open folder", "fr": "Ouvrir le dossier", "de": "Ordner öffnen",
        "es": "Abrir la carpeta", "fi": "Avaa kansio", "it": "Apri la cartella",
        "nb_NO": "Åpne mappen",
    },
    "Remove": {
        "en": "Remove", "fr": "Retirer", "de": "Entfernen", "es": "Quitar",
        "fi": "Poista", "it": "Rimuovi", "nb_NO": "Fjern",
    },
    "Removing": {
        "en": "Removing", "fr": "Retrait", "de": "Wird entfernt",
        "es": "Quitando", "fi": "Poistetaan", "it": "Rimozione",
        "nb_NO": "Fjerner",
    },
    "Accept": {
        "en": "Accept", "fr": "Accepter", "de": "Annehmen", "es": "Aceptar",
        "fi": "Hyväksy", "it": "Accetta", "nb_NO": "Godta",
    },
    "Decline": {
        "en": "Decline", "fr": "Refuser", "de": "Ablehnen", "es": "Rechazar",
        "fi": "Hylkää", "it": "Rifiuta", "nb_NO": "Avslå",
    },
    "Send": {
        "en": "Send", "fr": "Envoyer", "de": "Senden", "es": "Enviar",
        "fi": "Lähetä", "it": "Invia", "nb_NO": "Send",
    },
    "Send files": {
        "en": "Send files", "fr": "Envoyer des fichiers", "de": "Dateien senden",
        "es": "Enviar archivos", "fi": "Lähetä tiedostoja",
        "it": "Invia file", "nb_NO": "Send filer",
    },
    "Add files": {
        "en": "Add files", "fr": "Ajouter des fichiers", "de": "Dateien hinzufügen",
        "es": "Añadir archivos", "fi": "Lisää tiedostoja",
        "it": "Aggiungi file", "nb_NO": "Legg til filer",
    },
    "Add more files": {
        "en": "Add more files", "fr": "Ajouter d'autres fichiers",
        "de": "Weitere Dateien hinzufügen", "es": "Añadir más archivos",
        "fi": "Lisää tiedostoja", "it": "Aggiungi altri file",
        "nb_NO": "Legg til flere filer",
    },
    "Clear all": {
        "en": "Clear all", "fr": "Tout vider", "de": "Alles leeren",
        "es": "Vaciar todo", "fi": "Tyhjennä kaikki", "it": "Svuota tutto",
        "nb_NO": "Tøm alt",
    },
    "Clear selection": {
        "en": "Clear selection", "fr": "Vider la sélection",
        "de": "Auswahl leeren", "es": "Vaciar la selección",
        "fi": "Tyhjennä valinta", "it": "Svuota la selezione",
        "nb_NO": "Tøm utvalget",
    },
    "Clear history": {
        "en": "Clear history", "fr": "Effacer l'historique",
        "de": "Verlauf löschen", "es": "Borrar el historial",
        "fi": "Tyhjennä historia", "it": "Cancella la cronologia",
        "nb_NO": "Tøm historikken",
    },
    "Clearing history": {
        "en": "Clearing history", "fr": "Effacement de l'historique",
        "de": "Verlauf wird gelöscht", "es": "Borrando el historial",
        "fi": "Tyhjennetään historiaa", "it": "Cancellazione della cronologia",
        "nb_NO": "Tømmer historikken",
    },
    "Search every address": {
        "en": "Search every address", "fr": "Sonder toutes les adresses",
        "de": "Jede Adresse absuchen", "es": "Sondear todas las direcciones",
        "fi": "Kokeile kaikkia osoitteita", "it": "Sonda ogni indirizzo",
        "nb_NO": "Søk gjennom alle adresser",
    },
    "Stop searching": {
        "en": "Stop searching", "fr": "Arrêter la recherche",
        "de": "Suche abbrechen", "es": "Detener la búsqueda",
        "fi": "Lopeta etsintä", "it": "Ferma la ricerca",
        "nb_NO": "Stopp søket",
    },
    "Add by address": {
        "en": "Add by address", "fr": "Ajouter par adresse",
        "de": "Über Adresse hinzufügen", "es": "Añadir por dirección",
        "fi": "Lisää osoitteella", "it": "Aggiungi per indirizzo",
        "nb_NO": "Legg til via adresse",
    },
    "For a device on another network, behind a VPN, or on a Wi-Fi that keeps clients apart. Both plain and encrypted transports are tried.": {
        "en": "For a device on another network, behind a VPN, or on a Wi-Fi that keeps clients apart. Both plain and encrypted transports are tried.",
        "fr": "Pour un appareil sur un autre réseau, derrière un VPN, ou sur un Wi-Fi qui isole les clients entre eux. Les transports en clair et chiffré sont essayés tous les deux.",
        "de": "Für ein Gerät in einem anderen Netz, hinter einem VPN oder in einem WLAN, das Clients voneinander trennt. Beide Transporte werden versucht, verschlüsselt und unverschlüsselt.",
        "es": "Para un dispositivo en otra red, tras una VPN, o en un wifi que aísla a los clientes entre sí. Se prueban ambos transportes, cifrado y sin cifrar.",
        "fi": "Laitteelle joka on toisessa verkossa, VPN:n takana tai wifissä joka eristää asiakkaat toisistaan. Molemmat siirtotavat kokeillaan, salattu ja salaamaton.",
        "it": "Per un dispositivo su un'altra rete, dietro una VPN, o su un Wi-Fi che tiene separati i client. Si provano entrambi i trasporti, cifrato e in chiaro.",
        "nb_NO": "For en enhet på et annet nettverk, bak en VPN, eller på et wifi som holder klientene adskilt. Begge transportene prøves, kryptert og ukryptert.",
    },
    "Leave this alone unless the other device was moved off the standard port.": {
        "en": "Leave this alone unless the other device was moved off the standard port.",
        "fr": "À ne changer que si l'autre appareil a quitté le port standard.",
        "de": "Nur ändern, wenn das andere Gerät den Standardport verlassen hat.",
        "es": "No lo toques salvo que el otro dispositivo haya cambiado de puerto.",
        "fi": "Älä koske tähän, ellei toista laitetta ole siirretty pois vakioportista.",
        "it": "Da cambiare solo se l'altro dispositivo ha lasciato la porta standard.",
        "nb_NO": "La denne være med mindre den andre enheten har flyttet fra standardporten.",
    },
    "Look for it": {
        "en": "Look for it", "fr": "Le chercher", "de": "Danach suchen",
        "es": "Buscarlo", "fi": "Etsi se", "it": "Cercalo",
        "nb_NO": "Se etter den",
    },
    "Found %1": {
        "en": "Found %1", "fr": "%1 trouvé", "de": "%1 gefunden",
        "es": "Se encontró %1", "fi": "Löytyi: %1", "it": "Trovato %1",
        "nb_NO": "Fant %1",
    },
    "Nothing answered at that address": {
        "en": "Nothing answered at that address",
        "fr": "Rien n'a répondu à cette adresse",
        "de": "Unter dieser Adresse hat nichts geantwortet",
        "es": "Nada respondió en esa dirección",
        "fi": "Mikään ei vastannut siihen osoitteeseen",
        "it": "Nessuna risposta a quell'indirizzo",
        "nb_NO": "Ingenting svarte på den adressen",
    },
    "Remembered addresses": {
        "en": "Remembered addresses", "fr": "Adresses mémorisées",
        "de": "Gemerkte Adressen", "es": "Direcciones recordadas",
        "fi": "Muistetut osoitteet", "it": "Indirizzi memorizzati",
        "nb_NO": "Lagrede adresser",
    },
    "Forget": {
        "en": "Forget", "fr": "Oublier", "de": "Vergessen", "es": "Olvidar",
        "fi": "Unohda", "it": "Dimentica", "nb_NO": "Glem",
    },
    "Forgetting": {
        "en": "Forgetting", "fr": "Oubli", "de": "Wird vergessen",
        "es": "Olvidando", "fi": "Unohdetaan", "it": "Rimozione",
        "nb_NO": "Glemmer",
    },
    "Look again": {
        "en": "Look again", "fr": "Chercher à nouveau", "de": "Erneut suchen",
        "es": "Buscar de nuevo", "fi": "Etsi uudelleen",
        "it": "Cerca di nuovo", "nb_NO": "Se etter igjen",
    },
    "Suggest another": {
        "en": "Suggest another", "fr": "En proposer un autre",
        "de": "Anderen vorschlagen", "es": "Sugerir otro",
        "fi": "Ehdota toista", "it": "Proponine un altro",
        "nb_NO": "Foreslå et annet",
    },
    "Suggest another name": {
        "en": "Suggest another name", "fr": "Proposer un autre nom",
        "de": "Anderen Namen vorschlagen", "es": "Sugerir otro nombre",
        "fi": "Ehdota toista nimeä", "it": "Proponi un altro nome",
        "nb_NO": "Foreslå et annet navn",
    },
    "Device details": {
        "en": "Device details", "fr": "Détails de l'appareil",
        "de": "Gerätedetails", "es": "Detalles del dispositivo",
        "fi": "Laitteen tiedot", "it": "Dettagli del dispositivo",
        "nb_NO": "Enhetsdetaljer",
    },
    "Choose files to send": {
        "en": "Choose files to send", "fr": "Choisir les fichiers à envoyer",
        "de": "Dateien zum Senden wählen", "es": "Elegir archivos para enviar",
        "fi": "Valitse lähetettävät tiedostot", "it": "Scegli i file da inviare",
        "nb_NO": "Velg filer å sende",
    },
    "Send staged files": {
        "en": "Send staged files", "fr": "Envoyer les fichiers en attente",
        "de": "Bereitgelegte Dateien senden", "es": "Enviar los archivos preparados",
        "fi": "Lähetä valitut tiedostot", "it": "Invia i file preparati",
        "nb_NO": "Send de klargjorte filene",
    },
    "Select files to send": {
        "en": "Select files to send", "fr": "Sélectionner les fichiers à envoyer",
        "de": "Dateien zum Senden auswählen", "es": "Seleccionar archivos para enviar",
        "fi": "Valitse lähetettävät tiedostot",
        "it": "Seleziona i file da inviare", "nb_NO": "Velg filer å sende",
    },

    # --- main page ------------------------------------------------------
    "LocalSend": {
        "en": "LocalSend", "fr": "LocalSend", "de": "LocalSend",
        "es": "LocalSend", "fi": "LocalSend", "it": "LocalSend",
        "nb_NO": "LocalSend",
    },
    "Nearby devices": {
        "en": "Nearby devices", "fr": "Appareils à proximité",
        "de": "Geräte in der Nähe", "es": "Dispositivos cercanos",
        "fi": "Lähellä olevat laitteet", "it": "Dispositivi nelle vicinanze",
        "nb_NO": "Enheter i nærheten",
    },
    "Nobody yet": {
        "en": "Nobody yet", "fr": "Personne pour l'instant",
        "de": "Noch niemand", "es": "Nadie todavía",
        "fi": "Ei vielä ketään", "it": "Ancora nessuno",
        "nb_NO": "Ingen ennå",
    },
    "Open LocalSend on another device on the same network. It should turn up here within a few seconds. If it does not, pull down: Search every address goes through the whole subnet, and Add by address reaches one that is somewhere else entirely.": {
        "en": "Open LocalSend on another device on the same network. It should turn up here within a few seconds. If it does not, pull down: Search every address goes through the whole subnet, and Add by address reaches one that is somewhere else entirely.",
        "fr": "Ouvrez LocalSend sur un autre appareil du même réseau. Il devrait apparaître ici en quelques secondes. Sinon, tirez vers le bas : Sonder toutes les adresses parcourt tout le sous-réseau, et Ajouter par adresse atteint un appareil situé ailleurs.",
        "de": "Öffnen Sie LocalSend auf einem anderen Gerät im selben Netz. Es sollte hier binnen weniger Sekunden auftauchen. Wenn nicht, ziehen Sie herunter: Jede Adresse absuchen geht das ganze Subnetz durch, und Über Adresse hinzufügen erreicht ein Gerät, das ganz woanders steht.",
        "es": "Abre LocalSend en otro dispositivo de la misma red. Debería aparecer aquí en unos segundos. Si no, desliza hacia abajo: Sondear todas las direcciones recorre toda la subred, y Añadir por dirección llega a uno que está en otra parte.",
        "fi": "Avaa LocalSend toisella samassa verkossa olevalla laitteella. Sen pitäisi ilmestyä tähän muutamassa sekunnissa. Jos ei, vedä alas: Kokeile kaikkia osoitteita käy läpi koko aliverkon, ja Lisää osoitteella tavoittaa laitteen joka on aivan muualla.",
        "it": "Apri LocalSend su un altro dispositivo della stessa rete. Dovrebbe comparire qui in pochi secondi. Altrimenti tira verso il basso: Sonda ogni indirizzo percorre l'intera sottorete, e Aggiungi per indirizzo raggiunge un dispositivo che sta altrove.",
        "nb_NO": "Åpne LocalSend på en annen enhet på samme nettverk. Den bør dukke opp her i løpet av noen sekunder. Hvis ikke, dra ned: Søk gjennom alle adresser går gjennom hele subnettet, og Legg til via adresse når en enhet som står et helt annet sted.",
    },
    "Receiving is off. Turn it back on in Settings to be found.": {
        "en": "Receiving is off. Turn it back on in Settings to be found.",
        "fr": "La réception est désactivée. Réactivez-la dans les réglages pour être trouvé.",
        "de": "Der Empfang ist aus. Schalten Sie ihn in den Einstellungen wieder ein, um gefunden zu werden.",
        "es": "La recepción está desactivada. Vuelve a activarla en los ajustes para que te encuentren.",
        "fi": "Vastaanotto on pois päältä. Kytke se takaisin asetuksista, jotta sinut löydetään.",
        "it": "La ricezione è disattivata. Riattivala nelle impostazioni per essere trovato.",
        "nb_NO": "Mottak er av. Slå det på igjen i innstillingene for å bli funnet.",
    },
    "Receiving is off — others cannot send to you": {
        "en": "Receiving is off — others cannot send to you",
        "fr": "Réception désactivée — personne ne peut vous envoyer de fichiers",
        "de": "Empfang aus — niemand kann Ihnen etwas senden",
        "es": "Recepción desactivada: nadie puede enviarte nada",
        "fi": "Vastaanotto pois päältä — kukaan ei voi lähettää sinulle",
        "it": "Ricezione disattivata — nessuno può inviarti file",
        "nb_NO": "Mottak er av — ingen kan sende til deg",
    },
    "Not listening": {
        "en": "Not listening", "fr": "Pas à l'écoute", "de": "Nicht empfangsbereit",
        "es": "Sin escuchar", "fi": "Ei kuuntele", "it": "Non in ascolto",
        "nb_NO": "Lytter ikke",
    },
    "Port %1 is unavailable": {
        "en": "Port %1 is unavailable", "fr": "Le port %1 est indisponible",
        "de": "Port %1 ist nicht verfügbar", "es": "El puerto %1 no está disponible",
        "fi": "Portti %1 ei ole käytettävissä", "it": "La porta %1 non è disponibile",
        "nb_NO": "Port %1 er utilgjengelig",
    },
    "Ready on port %1": {
        "en": "Ready on port %1", "fr": "Prêt sur le port %1",
        "de": "Bereit auf Port %1", "es": "Listo en el puerto %1",
        "fi": "Valmiina portissa %1", "it": "Pronto sulla porta %1",
        "nb_NO": "Klar på port %1",
    },
    "Ready · %1 on port %2": {
        "en": "Ready · %1 on port %2", "fr": "Prêt · %1 sur le port %2",
        "de": "Bereit · %1 auf Port %2", "es": "Listo · %1 en el puerto %2",
        "fi": "Valmiina · %1 portissa %2", "it": "Pronto · %1 sulla porta %2",
        "nb_NO": "Klar · %1 på port %2",
    },
    "This network is blocking discovery. Pull down and choose Scan network.": {
        "en": "This network is blocking discovery. Pull down and choose Scan network.",
        "fr": "Ce réseau bloque la découverte. Tirez vers le bas et choisissez Balayer le réseau.",
        "de": "Dieses Netz blockiert die Gerätesuche. Ziehen Sie herunter und wählen Sie Netz absuchen.",
        "es": "Esta red bloquea el descubrimiento. Desliza hacia abajo y elige Explorar la red.",
        "fi": "Tämä verkko estää laitteiden löytämisen. Vedä alas ja valitse Etsi verkosta.",
        "it": "Questa rete blocca il rilevamento. Tira verso il basso e scegli Scansiona la rete.",
        "nb_NO": "Dette nettverket blokkerer oppdagelse. Dra ned og velg Søk i nettverket.",
    },
    "Scanning the network… %1%": {
        "en": "Scanning the network… %1%", "fr": "Balayage du réseau… %1 %",
        "de": "Netz wird abgesucht… %1 %", "es": "Explorando la red… %1 %",
        "fi": "Etsitään verkosta… %1 %", "it": "Scansione della rete… %1 %",
        "nb_NO": "Søker i nettverket… %1 %",
    },
    "Your device name": {
        "en": "Your device name", "fr": "Le nom de votre appareil",
        "de": "Name Ihres Geräts", "es": "El nombre de tu dispositivo",
        "fi": "Laitteesi nimi", "it": "Il nome del tuo dispositivo",
        "nb_NO": "Navnet på enheten din",
    },
    "Shown to other devices": {
        "en": "Shown to other devices", "fr": "Visible par les autres appareils",
        "de": "Für andere Geräte sichtbar", "es": "Visible para otros dispositivos",
        "fi": "Näkyy muille laitteille", "it": "Visibile agli altri dispositivi",
        "nb_NO": "Vises for andre enheter",
    },
    "Device name": {
        "en": "Device name", "fr": "Nom de l'appareil", "de": "Gerätename",
        "es": "Nombre del dispositivo", "fi": "Laitteen nimi",
        "it": "Nome del dispositivo", "nb_NO": "Enhetsnavn",
    },
    "Device": {
        "en": "Device", "fr": "Appareil", "de": "Gerät", "es": "Dispositivo",
        "fi": "Laite", "it": "Dispositivo", "nb_NO": "Enhet",
    },
    "Model": {
        "en": "Model", "fr": "Modèle", "de": "Modell", "es": "Modelo",
        "fi": "Malli", "it": "Modello", "nb_NO": "Modell",
    },
    "Type": {
        "en": "Type", "fr": "Type", "de": "Typ", "es": "Tipo",
        "fi": "Tyyppi", "it": "Tipo", "nb_NO": "Type",
    },
    "Address": {
        "en": "Address", "fr": "Adresse", "de": "Adresse", "es": "Dirección",
        "fi": "Osoite", "it": "Indirizzo", "nb_NO": "Adresse",
    },
    "Transport": {
        "en": "Transport", "fr": "Transport", "de": "Transport",
        "es": "Transporte", "fi": "Siirtotapa", "it": "Trasporto",
        "nb_NO": "Transport",
    },
    "Fingerprint": {
        "en": "Fingerprint", "fr": "Empreinte", "de": "Fingerabdruck",
        "es": "Huella", "fi": "Sormenjälki", "it": "Impronta",
        "nb_NO": "Fingeravtrykk",
    },
    "Unknown": {
        "en": "Unknown", "fr": "Inconnu", "de": "Unbekannt",
        "es": "Desconocido", "fi": "Tuntematon", "it": "Sconosciuto",
        "nb_NO": "Ukjent",
    },

    # --- tray and selection ------------------------------------------------
    "pick a device to send": {
        "en": "pick a device to send", "fr": "choisissez un appareil",
        "de": "Gerät zum Senden wählen", "es": "elige un dispositivo",
        "fi": "valitse laite", "it": "scegli un dispositivo",
        "nb_NO": "velg en enhet",
    },
    "Ready to send": {
        "en": "Ready to send", "fr": "Prêt à envoyer", "de": "Bereit zum Senden",
        "es": "Listo para enviar", "fi": "Valmiina lähetettäväksi",
        "it": "Pronto per l'invio", "nb_NO": "Klar til å sendes",
    },
    "Nothing staged": {
        "en": "Nothing staged", "fr": "Rien en attente", "de": "Nichts bereitgelegt",
        "es": "Nada preparado", "fi": "Ei mitään valittuna",
        "it": "Niente in attesa", "nb_NO": "Ingenting klargjort",
    },
    "Pull down to add files": {
        "en": "Pull down to add files", "fr": "Tirez vers le bas pour ajouter des fichiers",
        "de": "Herunterziehen, um Dateien hinzuzufügen",
        "es": "Desliza hacia abajo para añadir archivos",
        "fi": "Vedä alas lisätäksesi tiedostoja",
        "it": "Tira verso il basso per aggiungere file",
        "nb_NO": "Dra ned for å legge til filer",
    },

    # --- transfer ----------------------------------------------------------
    "Sending": {
        "en": "Sending", "fr": "Envoi", "de": "Senden", "es": "Enviando",
        "fi": "Lähetetään", "it": "Invio", "nb_NO": "Sender",
    },
    "Receiving": {
        "en": "Receiving", "fr": "Réception", "de": "Empfang",
        "es": "Recepción", "fi": "Vastaanotto", "it": "Ricezione",
        "nb_NO": "Mottak",
    },
    "Receiving off": {
        "en": "Receiving off", "fr": "Réception désactivée", "de": "Empfang aus",
        "es": "Recepción desactivada", "fi": "Vastaanotto pois",
        "it": "Ricezione disattivata", "nb_NO": "Mottak av",
    },
    "Incoming": {
        "en": "Incoming", "fr": "Entrant", "de": "Eingehend",
        "es": "Entrante", "fi": "Saapuva", "it": "In arrivo",
        "nb_NO": "Innkommende",
    },
    "Incoming files": {
        "en": "Incoming files", "fr": "Fichiers entrants",
        "de": "Eingehende Dateien", "es": "Archivos entrantes",
        "fi": "Saapuvat tiedostot", "it": "File in arrivo",
        "nb_NO": "Innkommende filer",
    },
    "What they are sending": {
        "en": "What they are sending", "fr": "Ce qui vous est envoyé",
        "de": "Was gesendet wird", "es": "Lo que te envían",
        "fi": "Mitä sinulle lähetetään", "it": "Che cosa ti stanno inviando",
        "nb_NO": "Hva som sendes",
    },
    "from %1": {
        "en": "from %1", "fr": "depuis %1", "de": "von %1", "es": "desde %1",
        "fi": "osoitteesta %1", "it": "da %1", "nb_NO": "fra %1",
    },
    "in %1": {
        "en": "in %1", "fr": "dans %1", "de": "in %1", "es": "en %1",
        "fi": "kansiossa %1", "it": "in %1", "nb_NO": "i %1",
    },
    "Saved to %1": {
        "en": "Saved to %1", "fr": "Enregistré dans %1",
        "de": "Gespeichert in %1", "es": "Se guardará en %1",
        "fi": "Tallennetaan kansioon %1", "it": "Salvato in %1",
        "nb_NO": "Lagres i %1",
    },
    "Files": {
        "en": "Files", "fr": "Fichiers", "de": "Dateien", "es": "Archivos",
        "fi": "Tiedostot", "it": "File", "nb_NO": "Filer",
    },
    "Starting…": {
        "en": "Starting…", "fr": "Démarrage…", "de": "Wird gestartet…",
        "es": "Empezando…", "fi": "Aloitetaan…", "it": "Avvio…",
        "nb_NO": "Starter…",
    },
    "%1 · %2 left": {
        "en": "%1 · %2 left", "fr": "%1 · %2 restant", "de": "%1 · noch %2",
        "es": "%1 · queda %2", "fi": "%1 · %2 jäljellä", "it": "%1 · %2 rimanenti",
        "nb_NO": "%1 · %2 igjen",
    },
    "Waiting for %1 to accept": {
        "en": "Waiting for %1 to accept", "fr": "En attente de l'accord de %1",
        "de": "Warten auf die Zustimmung von %1",
        "es": "Esperando a que %1 acepte", "fi": "Odotetaan, että %1 hyväksyy",
        "it": "In attesa che %1 accetti", "nb_NO": "Venter på at %1 godtar",
    },
    "Transfer failed": {
        "en": "Transfer failed", "fr": "Échec du transfert",
        "de": "Übertragung fehlgeschlagen", "es": "La transferencia falló",
        "fi": "Siirto epäonnistui", "it": "Trasferimento non riuscito",
        "nb_NO": "Overføringen mislyktes",
    },
    "Transfer stopped": {
        "en": "Transfer stopped", "fr": "Transfert arrêté",
        "de": "Übertragung angehalten", "es": "Transferencia detenida",
        "fi": "Siirto pysäytetty", "it": "Trasferimento interrotto",
        "nb_NO": "Overføringen ble stoppet",
    },
    "Transfer incomplete": {
        "en": "Transfer incomplete", "fr": "Transfert incomplet",
        "de": "Übertragung unvollständig", "es": "Transferencia incompleta",
        "fi": "Siirto jäi kesken", "it": "Trasferimento incompleto",
        "nb_NO": "Ufullstendig overføring",
    },
    "failed": {
        "en": "failed", "fr": "échec", "de": "fehlgeschlagen", "es": "fallido",
        "fi": "epäonnistui", "it": "non riuscito", "nb_NO": "mislyktes",
    },
    "skipped": {
        "en": "skipped", "fr": "ignoré", "de": "übersprungen", "es": "omitido",
        "fi": "ohitettu", "it": "saltato", "nb_NO": "hoppet over",
    },
    "incomplete": {
        "en": "incomplete", "fr": "incomplet", "de": "unvollständig",
        "es": "incompleto", "fi": "kesken", "it": "incompleto",
        "nb_NO": "ufullstendig",
    },
    "%1 wants to send you files": {
        "en": "%1 wants to send you files",
        "fr": "%1 veut vous envoyer des fichiers",
        "de": "%1 möchte Ihnen Dateien senden",
        "es": "%1 quiere enviarte archivos",
        "fi": "%1 haluaa lähettää sinulle tiedostoja",
        "it": "%1 vuole inviarti dei file",
        "nb_NO": "%1 vil sende deg filer",
    },

    # --- history -----------------------------------------------------------
    "From %1": {
        "en": "From %1", "fr": "De %1", "de": "Von %1", "es": "De %1",
        "fi": "Lähettäjä %1", "it": "Da %1", "nb_NO": "Fra %1",
    },
    "To %1": {
        "en": "To %1", "fr": "Vers %1", "de": "An %1", "es": "A %1",
        "fi": "Vastaanottaja %1", "it": "A %1", "nb_NO": "Til %1",
    },
    "+%1 more": {
        "en": "+%1 more", "fr": "+%1 autre(s)", "de": "+%1 weitere",
        "es": "+%1 más", "fi": "+%1 muuta", "it": "+%1 altri",
        "nb_NO": "+%1 til",
    },
    "Nothing yet": {
        "en": "Nothing yet", "fr": "Rien pour l'instant", "de": "Noch nichts",
        "es": "Nada todavía", "fi": "Ei vielä mitään", "it": "Ancora niente",
        "nb_NO": "Ingenting ennå",
    },
    "Transfers you send and receive will be listed here.": {
        "en": "Transfers you send and receive will be listed here.",
        "fr": "Les transferts envoyés et reçus apparaîtront ici.",
        "de": "Gesendete und empfangene Übertragungen erscheinen hier.",
        "es": "Las transferencias enviadas y recibidas aparecerán aquí.",
        "fi": "Lähettämäsi ja vastaanottamasi siirrot näkyvät tässä.",
        "it": "I trasferimenti inviati e ricevuti compariranno qui.",
        "nb_NO": "Overføringer du sender og mottar vises her.",
    },

    # --- PIN ----------------------------------------------------------------
    "PIN": {
        "en": "PIN", "fr": "Code PIN", "de": "PIN", "es": "PIN",
        "fi": "PIN-koodi", "it": "PIN", "nb_NO": "PIN",
    },
    "PIN required": {
        "en": "PIN required", "fr": "Code PIN requis", "de": "PIN erforderlich",
        "es": "Se requiere PIN", "fi": "PIN-koodi vaaditaan",
        "it": "PIN richiesto", "nb_NO": "PIN kreves",
    },
    "%1 is asking for a PIN before accepting files.": {
        "en": "%1 is asking for a PIN before accepting files.",
        "fr": "%1 demande un code PIN avant d'accepter des fichiers.",
        "de": "%1 verlangt eine PIN, bevor Dateien angenommen werden.",
        "es": "%1 pide un PIN antes de aceptar archivos.",
        "fi": "%1 pyytää PIN-koodia ennen tiedostojen hyväksymistä.",
        "it": "%1 chiede un PIN prima di accettare i file.",
        "nb_NO": "%1 ber om en PIN før filer godtas.",
    },
    "That code was not accepted. Try again.": {
        "en": "That code was not accepted. Try again.",
        "fr": "Ce code a été refusé. Réessayez.",
        "de": "Dieser Code wurde abgelehnt. Versuchen Sie es erneut.",
        "es": "Ese código no se aceptó. Inténtalo de nuevo.",
        "fi": "Koodia ei hyväksytty. Yritä uudelleen.",
        "it": "Questo codice non è stato accettato. Riprova.",
        "nb_NO": "Koden ble ikke godtatt. Prøv igjen.",
    },
    "4 to 8 digits": {
        "en": "4 to 8 digits", "fr": "4 à 8 chiffres", "de": "4 bis 8 Ziffern",
        "es": "De 4 a 8 dígitos", "fi": "4–8 numeroa", "it": "Da 4 a 8 cifre",
        "nb_NO": "4 til 8 sifre",
    },

    # --- settings -------------------------------------------------------------
    "This device": {
        "en": "This device", "fr": "Cet appareil", "de": "Dieses Gerät",
        "es": "Este dispositivo", "fi": "Tämä laite", "it": "Questo dispositivo",
        "nb_NO": "Denne enheten",
    },
    "What other devices call you.": {
        "en": "What other devices call you.",
        "fr": "Le nom sous lequel les autres appareils vous voient.",
        "de": "So nennen Sie andere Geräte.",
        "es": "Así te llaman los demás dispositivos.",
        "fi": "Tällä nimellä muut laitteet näkevät sinut.",
        "it": "Il nome con cui ti vedono gli altri dispositivi.",
        "nb_NO": "Slik ser andre enheter deg.",
    },
    "Allow incoming files": {
        "en": "Allow incoming files", "fr": "Autoriser les fichiers entrants",
        "de": "Eingehende Dateien zulassen", "es": "Permitir archivos entrantes",
        "fi": "Salli saapuvat tiedostot", "it": "Consenti i file in arrivo",
        "nb_NO": "Tillat innkommende filer",
    },
    "When off, this device stops announcing itself and refuses transfers.": {
        "en": "When off, this device stops announcing itself and refuses transfers.",
        "fr": "Désactivé, cet appareil cesse de s'annoncer et refuse les transferts.",
        "de": "Ist dies aus, meldet sich das Gerät nicht mehr und lehnt Übertragungen ab.",
        "es": "Si está desactivado, el dispositivo deja de anunciarse y rechaza las transferencias.",
        "fi": "Pois päältä laite lakkaa ilmoittamasta itsestään ja hylkää siirrot.",
        "it": "Se disattivato, il dispositivo smette di annunciarsi e rifiuta i trasferimenti.",
        "nb_NO": "Når dette er av, slutter enheten å kunngjøre seg og avviser overføringer.",
    },
    "Accept without asking": {
        "en": "Accept without asking", "fr": "Accepter sans demander",
        "de": "Ohne Nachfrage annehmen", "es": "Aceptar sin preguntar",
        "fi": "Hyväksy kysymättä", "it": "Accetta senza chiedere",
        "nb_NO": "Godta uten å spørre",
    },
    "Files are saved as soon as they arrive. Convenient at home, unwise on a network you share.": {
        "en": "Files are saved as soon as they arrive. Convenient at home, unwise on a network you share.",
        "fr": "Les fichiers sont enregistrés dès leur arrivée. Pratique chez soi, imprudent sur un réseau partagé.",
        "de": "Dateien werden sofort gespeichert. Zu Hause praktisch, in einem geteilten Netz unklug.",
        "es": "Los archivos se guardan nada más llegar. Cómodo en casa, imprudente en una red compartida.",
        "fi": "Tiedostot tallennetaan heti niiden saapuessa. Kotona kätevää, jaetussa verkossa harkitsematonta.",
        "it": "I file vengono salvati appena arrivano. Comodo a casa, imprudente su una rete condivisa.",
        "nb_NO": "Filer lagres med én gang de kommer. Praktisk hjemme, uklokt på et delt nettverk.",
    },
    "Require a PIN": {
        "en": "Require a PIN", "fr": "Exiger un code PIN", "de": "PIN verlangen",
        "es": "Exigir un PIN", "fi": "Vaadi PIN-koodi", "it": "Richiedi un PIN",
        "nb_NO": "Krev en PIN",
    },
    "Senders must enter this code before you are even asked.": {
        "en": "Senders must enter this code before you are even asked.",
        "fr": "L'expéditeur doit saisir ce code avant même que la question vous soit posée.",
        "de": "Der Absender muss diesen Code eingeben, bevor Sie überhaupt gefragt werden.",
        "es": "Quien envía debe introducir este código antes de que se te pregunte siquiera.",
        "fi": "Lähettäjän on annettava tämä koodi ennen kuin sinulta edes kysytään.",
        "it": "Chi invia deve inserire questo codice prima ancora che ti venga chiesto.",
        "nb_NO": "Avsenderen må oppgi denne koden før du i det hele tatt blir spurt.",
    },
    "Port": {
        "en": "Port", "fr": "Port", "de": "Port", "es": "Puerto",
        "fi": "Portti", "it": "Porta", "nb_NO": "Port",
    },
    "Listening port": {
        "en": "Listening port", "fr": "Port d'écoute", "de": "Empfangsport",
        "es": "Puerto de escucha", "fi": "Kuunneltava portti",
        "it": "Porta di ascolto", "nb_NO": "Lytteport",
    },
    "53317 is the standard. Change it only if something else is using the port.": {
        "en": "53317 is the standard. Change it only if something else is using the port.",
        "fr": "53317 est le port standard. Ne le changez que si un autre programme l'utilise.",
        "de": "53317 ist der Standard. Ändern Sie ihn nur, wenn etwas anderes den Port belegt.",
        "es": "53317 es el estándar. Cámbialo solo si otra cosa está usando el puerto.",
        "fi": "53317 on vakioportti. Vaihda se vain, jos jokin muu käyttää sitä.",
        "it": "53317 è la porta standard. Cambiala solo se è già occupata da altro.",
        "nb_NO": "53317 er standarden. Endre den bare hvis noe annet bruker porten.",
    },
    "Other LocalSend devices look on 53317 by default. A different port still works, but only if the other side is told about it.": {
        "en": "Other LocalSend devices look on 53317 by default. A different port still works, but only if the other side is told about it.",
        "fr": "Les autres appareils LocalSend cherchent sur 53317 par défaut. Un autre port fonctionne, mais seulement si l'autre côté en est informé.",
        "de": "Andere LocalSend-Geräte suchen standardmäßig auf 53317. Ein anderer Port funktioniert, aber nur wenn die Gegenseite davon weiß.",
        "es": "Los demás dispositivos LocalSend buscan en el 53317 por omisión. Otro puerto funciona, pero solo si el otro lado lo sabe.",
        "fi": "Muut LocalSend-laitteet etsivät oletuksena portista 53317. Muukin portti toimii, mutta vain jos toinen osapuoli tietää siitä.",
        "it": "Gli altri dispositivi LocalSend cercano sulla 53317 per impostazione predefinita. Un'altra porta funziona, ma solo se l'altro lato ne è informato.",
        "nb_NO": "Andre LocalSend-enheter ser på 53317 som standard. En annen port fungerer, men bare hvis den andre siden får vite om den.",
    },
    "Saving": {
        "en": "Saving", "fr": "Enregistrement", "de": "Speichern",
        "es": "Guardado", "fi": "Tallennus", "it": "Salvataggio",
        "nb_NO": "Lagring",
    },
    "Save to": {
        "en": "Save to", "fr": "Enregistrer dans", "de": "Speichern unter",
        "es": "Guardar en", "fi": "Tallenna kansioon", "it": "Salva in",
        "nb_NO": "Lagre i",
    },
    "Where to save incoming files": {
        "en": "Where to save incoming files",
        "fr": "Où enregistrer les fichiers reçus",
        "de": "Wohin eingehende Dateien gespeichert werden",
        "es": "Dónde guardar los archivos recibidos",
        "fi": "Mihin saapuvat tiedostot tallennetaan",
        "it": "Dove salvare i file ricevuti",
        "nb_NO": "Hvor innkommende filer skal lagres",
    },
    "A folder per sender": {
        "en": "A folder per sender", "fr": "Un dossier par expéditeur",
        "de": "Ein Ordner je Absender", "es": "Una carpeta por remitente",
        "fi": "Oma kansio kullekin lähettäjälle",
        "it": "Una cartella per mittente", "nb_NO": "En mappe per avsender",
    },
    "Received files go into a subfolder named after the device that sent them.": {
        "en": "Received files go into a subfolder named after the device that sent them.",
        "fr": "Les fichiers reçus vont dans un sous-dossier au nom de l'appareil expéditeur.",
        "de": "Empfangene Dateien landen in einem Unterordner mit dem Namen des Absendergeräts.",
        "es": "Los archivos recibidos van a una subcarpeta con el nombre del dispositivo que los envió.",
        "fi": "Vastaanotetut tiedostot menevät alikansioon, joka on nimetty lähettäneen laitteen mukaan.",
        "it": "I file ricevuti finiscono in una sottocartella con il nome del dispositivo che li ha inviati.",
        "nb_NO": "Mottatte filer havner i en undermappe oppkalt etter enheten som sendte dem.",
    },
    "While transferring": {
        "en": "While transferring", "fr": "Pendant les transferts",
        "de": "Während der Übertragung", "es": "Durante las transferencias",
        "fi": "Siirron aikana", "it": "Durante i trasferimenti",
        "nb_NO": "Under overføring",
    },
    "Notify me": {
        "en": "Notify me", "fr": "Me notifier", "de": "Benachrichtigen",
        "es": "Avisarme", "fi": "Ilmoita minulle", "it": "Avvisami",
        "nb_NO": "Varsle meg",
    },
    "A notification when a transfer arrives or finishes in the background.": {
        "en": "A notification when a transfer arrives or finishes in the background.",
        "fr": "Une notification lorsqu'un transfert arrive ou se termine en arrière-plan.",
        "de": "Eine Benachrichtigung, wenn im Hintergrund eine Übertragung eintrifft oder endet.",
        "es": "Una notificación cuando una transferencia llega o termina en segundo plano.",
        "fi": "Ilmoitus, kun siirto saapuu tai päättyy taustalla.",
        "it": "Una notifica quando un trasferimento arriva o finisce in secondo piano.",
        "nb_NO": "Et varsel når en overføring kommer eller blir ferdig i bakgrunnen.",
    },
    "Keep going with the screen off": {
        "en": "Keep going with the screen off",
        "fr": "Continuer écran éteint",
        "de": "Bei ausgeschaltetem Bildschirm weiterlaufen",
        "es": "Continuar con la pantalla apagada",
        "fi": "Jatka näytön ollessa sammuksissa",
        "it": "Continua a schermo spento",
        "nb_NO": "Fortsett med skjermen av",
    },
    "Stops the device suspending mid-transfer. Uses more battery.": {
        "en": "Stops the device suspending mid-transfer. Uses more battery.",
        "fr": "Empêche la mise en veille en plein transfert. Consomme plus de batterie.",
        "de": "Verhindert, dass das Gerät mitten in einer Übertragung schlafen geht. Braucht mehr Akku.",
        "es": "Evita que el dispositivo se suspenda a mitad de una transferencia. Gasta más batería.",
        "fi": "Estää laitetta siirtymästä lepotilaan kesken siirron. Kuluttaa enemmän akkua.",
        "it": "Impedisce al dispositivo di sospendersi durante un trasferimento. Consuma più batteria.",
        "nb_NO": "Hindrer at enheten går i dvale midt i en overføring. Bruker mer batteri.",
    },
    "Keep a history": {
        "en": "Keep a history", "fr": "Conserver un historique",
        "de": "Verlauf führen", "es": "Guardar un historial",
        "fi": "Pidä historiaa", "it": "Conserva una cronologia",
        "nb_NO": "Før en historikk",
    },
    "Records what was sent and received, and where it was saved.": {
        "en": "Records what was sent and received, and where it was saved.",
        "fr": "Note ce qui a été envoyé et reçu, et où cela a été enregistré.",
        "de": "Hält fest, was gesendet und empfangen wurde und wo es gespeichert ist.",
        "es": "Anota lo que se envió y se recibió, y dónde se guardó.",
        "fi": "Kirjaa mitä lähetettiin ja vastaanotettiin ja minne se tallennettiin.",
        "it": "Registra che cosa è stato inviato e ricevuto, e dove è stato salvato.",
        "nb_NO": "Noterer hva som ble sendt og mottatt, og hvor det ble lagret.",
    },
    "Language": {
        "en": "Language", "fr": "Langue", "de": "Sprache", "es": "Idioma",
        "fi": "Kieli", "it": "Lingua", "nb_NO": "Språk",
    },
    "Interface language": {
        "en": "Interface language", "fr": "Langue de l'interface",
        "de": "Sprache der Oberfläche", "es": "Idioma de la interfaz",
        "fi": "Käyttöliittymän kieli", "it": "Lingua dell'interfaccia",
        "nb_NO": "Grensesnittspråk",
    },
    "The app reloads when this changes.": {
        "en": "The app reloads when this changes.",
        "fr": "L'application se recharge lors du changement.",
        "de": "Die App lädt bei einer Änderung neu.",
        "es": "La aplicación se recarga al cambiarlo.",
        "fi": "Sovellus latautuu uudelleen, kun tämä muuttuu.",
        "it": "L'applicazione si ricarica quando questo cambia.",
        "nb_NO": "Appen lastes på nytt når dette endres.",
    },

    # --- about ----------------------------------------------------------------
    "Version": {
        "en": "Version", "fr": "Version", "de": "Version", "es": "Versión",
        "fi": "Versio", "it": "Versione", "nb_NO": "Versjon",
    },
    "Protocol": {
        "en": "Protocol", "fr": "Protocole", "de": "Protokoll",
        "es": "Protocolo", "fi": "Protokolla", "it": "Protocollo",
        "nb_NO": "Protokoll",
    },
    "LocalSend v%1": {
        "en": "LocalSend v%1", "fr": "LocalSend v%1", "de": "LocalSend v%1",
        "es": "LocalSend v%1", "fi": "LocalSend v%1", "it": "LocalSend v%1",
        "nb_NO": "LocalSend v%1",
    },
    "Good to know": {
        "en": "Good to know", "fr": "Bon à savoir", "de": "Gut zu wissen",
        "es": "Conviene saber", "fi": "Hyvä tietää", "it": "Da sapere",
        "nb_NO": "Verdt å vite",
    },
    "Links": {
        "en": "Links", "fr": "Liens", "de": "Links", "es": "Enlaces",
        "fi": "Linkit", "it": "Collegamenti", "nb_NO": "Lenker",
    },
    "Source and issues": {
        "en": "Source and issues", "fr": "Code source et signalements",
        "de": "Quellcode und Fehlermeldungen", "es": "Código y problemas",
        "fi": "Lähdekoodi ja virheraportit", "it": "Codice e segnalazioni",
        "nb_NO": "Kildekode og feilmeldinger",
    },
    "The LocalSend project": {
        "en": "The LocalSend project", "fr": "Le projet LocalSend",
        "de": "Das LocalSend-Projekt", "es": "El proyecto LocalSend",
        "fi": "LocalSend-projekti", "it": "Il progetto LocalSend",
        "nb_NO": "LocalSend-prosjektet",
    },
    "An unofficial LocalSend client for Sailfish OS. Files go straight from one device to the other over your own network — no account, no server, no Internet connection needed.": {
        "en": "An unofficial LocalSend client for Sailfish OS. Files go straight from one device to the other over your own network — no account, no server, no Internet connection needed.",
        "fr": "Un client LocalSend non officiel pour Sailfish OS. Les fichiers passent directement d'un appareil à l'autre sur votre propre réseau — sans compte, sans serveur, sans connexion Internet.",
        "de": "Ein inoffizieller LocalSend-Client für Sailfish OS. Dateien gehen direkt von einem Gerät zum anderen über Ihr eigenes Netz — ohne Konto, ohne Server, ohne Internetverbindung.",
        "es": "Un cliente LocalSend no oficial para Sailfish OS. Los archivos van directos de un dispositivo a otro por tu propia red: sin cuenta, sin servidor, sin conexión a Internet.",
        "fi": "Epävirallinen LocalSend-asiakasohjelma Sailfish OS:lle. Tiedostot siirtyvät suoraan laitteelta toiselle omassa verkossasi — ilman tiliä, palvelinta tai internetyhteyttä.",
        "it": "Un client LocalSend non ufficiale per Sailfish OS. I file passano direttamente da un dispositivo all'altro sulla tua rete — senza account, senza server, senza connessione a Internet.",
        "nb_NO": "En uoffisiell LocalSend-klient for Sailfish OS. Filer går rett fra én enhet til en annen over ditt eget nettverk — uten konto, uten server, uten internettforbindelse.",
    },
    "Transfers are encrypted between the two devices with a certificate this phone generated for itself. There is no certificate authority on a local network, so what identifies a device is the fingerprint above: it travels in every announcement, and a device presenting anything else is refused before a single byte is sent.": {
        "en": "Transfers are encrypted between the two devices with a certificate this phone generated for itself. There is no certificate authority on a local network, so what identifies a device is the fingerprint above: it travels in every announcement, and a device presenting anything else is refused before a single byte is sent.",
        "fr": "Les transferts sont chiffrés entre les deux appareils avec un certificat que ce téléphone a généré lui-même. Il n'existe pas d'autorité de certification sur un réseau local : ce qui identifie un appareil, c'est l'empreinte ci-dessus. Elle voyage dans chaque annonce, et un appareil qui en présente une autre est refusé avant le moindre octet.",
        "de": "Übertragungen werden zwischen den beiden Geräten mit einem Zertifikat verschlüsselt, das dieses Telefon selbst erzeugt hat. In einem lokalen Netz gibt es keine Zertifizierungsstelle: Was ein Gerät ausweist, ist der Fingerabdruck oben. Er steht in jeder Ankündigung, und ein Gerät, das etwas anderes vorlegt, wird abgewiesen, bevor ein einziges Byte fließt.",
        "es": "Las transferencias van cifradas entre los dos dispositivos con un certificado que este teléfono generó para sí mismo. En una red local no hay ninguna autoridad de certificación: lo que identifica a un dispositivo es la huella de arriba. Viaja en cada anuncio, y un dispositivo que presente otra cosa se rechaza antes de enviar un solo byte.",
        "fi": "Siirrot salataan laitteiden välillä varmenteella, jonka tämä puhelin loi itselleen. Paikallisverkossa ei ole varmenneviranomaista: laitteen tunnistaa yllä oleva sormenjälki. Se kulkee jokaisessa ilmoituksessa, ja laite joka esittää jotain muuta hylätään ennen yhtäkään tavua.",
        "it": "I trasferimenti sono cifrati fra i due dispositivi con un certificato che questo telefono ha generato da sé. Su una rete locale non esiste alcuna autorità di certificazione: ciò che identifica un dispositivo è l'impronta qui sopra. Viaggia in ogni annuncio, e un dispositivo che ne presenta un'altra viene rifiutato prima di un solo byte.",
        "nb_NO": "Overføringer krypteres mellom de to enhetene med et sertifikat denne telefonen har laget selv. På et lokalt nettverk finnes ingen sertifikatmyndighet: det som identifiserer en enhet, er fingeravtrykket over. Det følger med hver kunngjøring, og en enhet som viser noe annet, avvises før en eneste byte sendes.",
    },
    "Encryption is off, so transfers use plain HTTP on port %1 and are readable by anyone who can watch the network. On a home network or your own hotspot that is nobody; on café or office Wi-Fi it may not be.": {
        "en": "Encryption is off, so transfers use plain HTTP on port %1 and are readable by anyone who can watch the network. On a home network or your own hotspot that is nobody; on café or office Wi-Fi it may not be.",
        "fr": "Le chiffrement est désactivé : les transferts passent en HTTP en clair sur le port %1 et sont lisibles par quiconque peut observer le réseau. Chez vous ou sur votre partage de connexion, cela ne concerne personne ; sur le Wi-Fi d'un café ou d'un bureau, peut-être si.",
        "de": "Die Verschlüsselung ist aus: Übertragungen laufen über unverschlüsseltes HTTP auf Port %1 und sind für jeden lesbar, der das Netz mitlesen kann. Im Heimnetz oder am eigenen Hotspot ist das niemand, im Café- oder Büro-WLAN womöglich schon.",
        "es": "El cifrado está desactivado: las transferencias usan HTTP sin cifrar en el puerto %1 y las puede leer cualquiera que observe la red. En casa o en tu propio punto de acceso no es nadie; en el wifi de una cafetería o una oficina puede que sí.",
        "fi": "Salaus on pois päältä: siirrot kulkevat salaamattomana HTTP:nä portissa %1 ja ovat kenen tahansa verkkoa seuraavan luettavissa. Kotiverkossa tai omassa jakoyhteydessä se ei ole kukaan; kahvilan tai toimiston wifissä ehkä on.",
        "it": "La cifratura è disattivata: i trasferimenti usano HTTP in chiaro sulla porta %1 e sono leggibili da chiunque possa osservare la rete. A casa o sul proprio hotspot non è nessuno; sul Wi-Fi di un bar o di un ufficio potrebbe esserlo.",
        "nb_NO": "Kryptering er av: overføringer går som ukryptert HTTP på port %1 og kan leses av alle som følger med på nettverket. Hjemme eller på ditt eget delte nett er det ingen; på kafé- eller kontor-wifi kan det være noen.",
    },
    "Encrypt transfers": {
        "en": "Encrypt transfers", "fr": "Chiffrer les transferts",
        "de": "Übertragungen verschlüsseln", "es": "Cifrar las transferencias",
        "fi": "Salaa siirrot", "it": "Cifra i trasferimenti",
        "nb_NO": "Krypter overføringer",
    },
    "Files are encrypted between the two devices. Turning this off makes every transfer readable by anyone on the same network.": {
        "en": "Files are encrypted between the two devices. Turning this off makes every transfer readable by anyone on the same network.",
        "fr": "Les fichiers sont chiffrés entre les deux appareils. Désactiver rend chaque transfert lisible par n'importe qui sur le même réseau.",
        "de": "Dateien werden zwischen den beiden Geräten verschlüsselt. Ausgeschaltet ist jede Übertragung für jeden im selben Netz lesbar.",
        "es": "Los archivos se cifran entre los dos dispositivos. Al desactivarlo, cualquiera en la misma red puede leer cada transferencia.",
        "fi": "Tiedostot salataan laitteiden välillä. Pois päältä jokainen siirto on kenen tahansa samassa verkossa olevan luettavissa.",
        "it": "I file sono cifrati fra i due dispositivi. Disattivandolo, ogni trasferimento diventa leggibile da chiunque sia sulla stessa rete.",
        "nb_NO": "Filer krypteres mellom de to enhetene. Slår du det av, kan hvem som helst på samme nettverk lese hver overføring.",
    },
    "This device is identified by the fingerprint of its certificate, so changing this setting makes it look like a new device to everyone else.": {
        "en": "This device is identified by the fingerprint of its certificate, so changing this setting makes it look like a new device to everyone else.",
        "fr": "Cet appareil est identifié par l'empreinte de son certificat : changer ce réglage le fait apparaître comme un nouvel appareil pour tous les autres.",
        "de": "Dieses Gerät wird über den Fingerabdruck seines Zertifikats erkannt, eine Änderung hier lässt es für alle anderen wie ein neues Gerät aussehen.",
        "es": "Este dispositivo se identifica por la huella de su certificado, así que cambiar este ajuste hace que los demás lo vean como un dispositivo nuevo.",
        "fi": "Tämä laite tunnistetaan varmenteensa sormenjäljestä, joten tämän asetuksen muuttaminen saa sen näyttämään muille uudelta laitteelta.",
        "it": "Questo dispositivo è identificato dall'impronta del suo certificato: cambiare questa impostazione lo fa apparire agli altri come un dispositivo nuovo.",
        "nb_NO": "Denne enheten identifiseres av fingeravtrykket til sertifikatet sitt, så å endre denne innstillingen får den til å se ut som en ny enhet for alle andre.",
    },
    "Unavailable on this device: %1": {
        "en": "Unavailable on this device: %1",
        "fr": "Indisponible sur cet appareil : %1",
        "de": "Auf diesem Gerät nicht verfügbar: %1",
        "es": "No disponible en este dispositivo: %1",
        "fi": "Ei käytettävissä tällä laitteella: %1",
        "it": "Non disponibile su questo dispositivo: %1",
        "nb_NO": "Ikke tilgjengelig på denne enheten: %1",
    },
    "This name has used a different key before": {
        "en": "This name has used a different key before",
        "fr": "Ce nom a déjà utilisé une autre clé",
        "de": "Dieser Name hat schon einen anderen Schlüssel benutzt",
        "es": "Este nombre ya usó otra clave",
        "fi": "Tämä nimi on käyttänyt aiemmin toista avainta",
        "it": "Questo nome ha già usato un'altra chiave",
        "nb_NO": "Dette navnet har brukt en annen nøkkel før",
    },
    "Send anyway?": {
        "en": "Send anyway?", "fr": "Envoyer quand même ?",
        "de": "Trotzdem senden?", "es": "¿Enviar de todos modos?",
        "fi": "Lähetetäänkö silti?", "it": "Inviare comunque?",
        "nb_NO": "Sende likevel?",
    },
    "Send anyway": {
        "en": "Send anyway", "fr": "Envoyer quand même",
        "de": "Trotzdem senden", "es": "Enviar de todos modos",
        "fi": "Lähetä silti", "it": "Invia comunque",
        "nb_NO": "Send likevel",
    },
    "You have sent to a device called %1 before, but it used a different key. Either it was reinstalled, or something else on this network is using its name.": {
        "en": "You have sent to a device called %1 before, but it used a different key. Either it was reinstalled, or something else on this network is using its name.",
        "fr": "Vous avez déjà envoyé à un appareil nommé %1, mais il utilisait une autre clé. Soit il a été réinstallé, soit autre chose sur ce réseau porte son nom.",
        "de": "Sie haben schon an ein Gerät namens %1 gesendet, aber mit einem anderen Schlüssel. Entweder wurde es neu installiert, oder etwas anderes in diesem Netz benutzt seinen Namen.",
        "es": "Ya has enviado a un dispositivo llamado %1, pero usaba otra clave. O se reinstaló, o alguna otra cosa en esta red está usando su nombre.",
        "fi": "Olet lähettänyt aiemmin laitteelle nimeltä %1, mutta se käytti toista avainta. Joko se on asennettu uudelleen tai jokin muu tässä verkossa käyttää sen nimeä.",
        "it": "Hai già inviato a un dispositivo chiamato %1, ma usava un'altra chiave. O è stato reinstallato, o qualcos'altro su questa rete ne sta usando il nome.",
        "nb_NO": "Du har sendt til en enhet som heter %1 før, men den brukte en annen nøkkel. Enten er den installert på nytt, eller så bruker noe annet på nettverket navnet dens.",
    },
    "Compare the fingerprint against the other device before continuing. Sending confirms the new key and it will not ask again.": {
        "en": "Compare the fingerprint against the other device before continuing. Sending confirms the new key and it will not ask again.",
        "fr": "Comparez l'empreinte avec celle affichée sur l'autre appareil avant de continuer. Envoyer confirme la nouvelle clé et la question ne sera plus posée.",
        "de": "Vergleichen Sie den Fingerabdruck mit dem des anderen Geräts, bevor Sie fortfahren. Senden bestätigt den neuen Schlüssel, und es wird nicht wieder gefragt.",
        "es": "Compara la huella con la del otro dispositivo antes de continuar. Enviar confirma la nueva clave y no se volverá a preguntar.",
        "fi": "Vertaa sormenjälkeä toisen laitteen näyttämään ennen jatkamista. Lähettäminen vahvistaa uuden avaimen eikä asiaa kysytä uudelleen.",
        "it": "Confronta l'impronta con quella dell'altro dispositivo prima di continuare. Inviare conferma la nuova chiave e non verrà più chiesto.",
        "nb_NO": "Sammenlign fingeravtrykket med det den andre enheten viser før du fortsetter. Å sende bekrefter den nye nøkkelen, og du blir ikke spurt igjen.",
    },
    "Now": {
        "en": "Now", "fr": "Maintenant", "de": "Jetzt", "es": "Ahora",
        "fi": "Nyt", "it": "Adesso", "nb_NO": "Nå",
    },
    "Before": {
        "en": "Before", "fr": "Avant", "de": "Vorher", "es": "Antes",
        "fi": "Aiemmin", "it": "Prima", "nb_NO": "Før",
    },
    "Seen before": {
        "en": "Seen before", "fr": "Déjà rencontré", "de": "Schon gesehen",
        "es": "Visto antes", "fi": "Tavattu aiemmin", "it": "Già incontrato",
        "nb_NO": "Sett før",
    },
    "Yes, key matches": {
        "en": "Yes, key matches", "fr": "Oui, la clé correspond",
        "de": "Ja, Schlüssel stimmt", "es": "Sí, la clave coincide",
        "fi": "Kyllä, avain täsmää", "it": "Sì, la chiave corrisponde",
        "nb_NO": "Ja, nøkkelen stemmer",
    },
    "First time": {
        "en": "First time", "fr": "Première fois", "de": "Zum ersten Mal",
        "es": "Primera vez", "fi": "Ensimmäinen kerta", "it": "Prima volta",
        "nb_NO": "Første gang",
    },
    "The only thing that identifies a device. Compare it against what the other device shows to be certain who you are talking to.": {
        "en": "The only thing that identifies a device. Compare it against what the other device shows to be certain who you are talking to.",
        "fr": "La seule chose qui identifie un appareil. Comparez-la avec ce qu'affiche l'autre appareil pour être certain de à qui vous parlez.",
        "de": "Das Einzige, was ein Gerät ausweist. Vergleichen Sie ihn mit dem, was das andere Gerät anzeigt, um sicher zu sein, mit wem Sie sprechen.",
        "es": "Lo único que identifica a un dispositivo. Compárala con la que muestra el otro para estar seguro de con quién hablas.",
        "fi": "Ainoa asia joka tunnistaa laitteen. Vertaa sitä toisen laitteen näyttämään ollaksesi varma kenen kanssa puhut.",
        "it": "L'unica cosa che identifica un dispositivo. Confrontala con quella mostrata dall'altro per essere certo con chi stai parlando.",
        "nb_NO": "Det eneste som identifiserer en enhet. Sammenlign det med det den andre enheten viser, for å være sikker på hvem du snakker med.",
    },
    "A device with this name used a different key before. Nothing stops one device on a network from announcing another one's name.": {
        "en": "A device with this name used a different key before. Nothing stops one device on a network from announcing another one's name.",
        "fr": "Un appareil de ce nom utilisait une autre clé auparavant. Rien n'empêche un appareil d'un réseau d'annoncer le nom d'un autre.",
        "de": "Ein Gerät dieses Namens hat vorher einen anderen Schlüssel benutzt. Nichts hindert ein Gerät im Netz daran, den Namen eines anderen anzukündigen.",
        "es": "Un dispositivo con este nombre usaba otra clave antes. Nada impide que un dispositivo de la red anuncie el nombre de otro.",
        "fi": "Tämän niminen laite käytti aiemmin toista avainta. Mikään ei estä verkossa olevaa laitetta ilmoittamasta toisen nimeä.",
        "it": "Un dispositivo con questo nome usava un'altra chiave. Nulla impedisce a un dispositivo in rete di annunciare il nome di un altro.",
        "nb_NO": "En enhet med dette navnet brukte en annen nøkkel før. Ingenting hindrer en enhet på nettverket i å kunngjøre en annens navn.",
    },
    "What other devices know this one by. Read it out to somebody to let them confirm it really is you they are sending to.": {
        "en": "What other devices know this one by. Read it out to somebody to let them confirm it really is you they are sending to.",
        "fr": "Ce par quoi les autres appareils reconnaissent celui-ci. Lisez-la à voix haute pour permettre à quelqu'un de confirmer que c'est bien à vous qu'il envoie.",
        "de": "Woran andere Geräte dieses erkennen. Lesen Sie ihn jemandem vor, damit er bestätigen kann, dass er wirklich an Sie sendet.",
        "es": "Aquello por lo que los demás dispositivos reconocen este. Léesela a alguien para que confirme que de verdad te está enviando a ti.",
        "fi": "Tästä muut laitteet tunnistavat tämän. Lue se jollekulle, jotta hän voi varmistaa lähettävänsä juuri sinulle.",
        "it": "Ciò da cui gli altri dispositivi riconoscono questo. Leggila a qualcuno perché possa confermare che sta inviando davvero a te.",
        "nb_NO": "Det andre enheter kjenner denne igjen på. Les det opp for noen så de kan bekrefte at det virkelig er deg de sender til.",
    },
    "Security": {
        "en": "Security", "fr": "Sécurité", "de": "Sicherheit",
        "es": "Seguridad", "fi": "Turvallisuus", "it": "Sicurezza",
        "nb_NO": "Sikkerhet",
    },
    "Encrypted": {
        "en": "Encrypted", "fr": "Chiffré", "de": "Verschlüsselt",
        "es": "Cifrado", "fi": "Salattu", "it": "Cifrato",
        "nb_NO": "Kryptert",
    },
    "Not encrypted": {
        "en": "Not encrypted", "fr": "Non chiffré", "de": "Unverschlüsselt",
        "es": "Sin cifrar", "fi": "Salaamaton", "it": "Non cifrato",
        "nb_NO": "Ukryptert",
    },
    "HTTPS (encrypted)": {
        "en": "HTTPS (encrypted)", "fr": "HTTPS (chiffré)",
        "de": "HTTPS (verschlüsselt)", "es": "HTTPS (cifrado)",
        "fi": "HTTPS (salattu)", "it": "HTTPS (cifrato)",
        "nb_NO": "HTTPS (kryptert)",
    },
    "HTTP (not encrypted)": {
        "en": "HTTP (not encrypted)", "fr": "HTTP (non chiffré)",
        "de": "HTTP (unverschlüsselt)", "es": "HTTP (sin cifrar)",
        "fi": "HTTP (salaamaton)", "it": "HTTP (non cifrato)",
        "nb_NO": "HTTP (ukryptert)",
    },
    "Set a PIN": {
        "en": "Set a PIN", "fr": "Définir un code PIN", "de": "PIN festlegen",
        "es": "Establecer un PIN", "fi": "Aseta PIN-koodi",
        "it": "Imposta un PIN", "nb_NO": "Angi en PIN",
    },
    "Change the PIN": {
        "en": "Change the PIN", "fr": "Changer le code PIN", "de": "PIN ändern",
        "es": "Cambiar el PIN", "fi": "Vaihda PIN-koodi",
        "it": "Cambia il PIN", "nb_NO": "Endre PIN",
    },
    "Remove the PIN": {
        "en": "Remove the PIN", "fr": "Supprimer le code PIN",
        "de": "PIN entfernen", "es": "Quitar el PIN",
        "fi": "Poista PIN-koodi", "it": "Rimuovi il PIN",
        "nb_NO": "Fjern PIN-en",
    },
    "Set": {
        "en": "Set", "fr": "Défini", "de": "Festgelegt", "es": "Establecido",
        "fi": "Asetettu", "it": "Impostato", "nb_NO": "Angitt",
    },
    "Not set": {
        "en": "Not set", "fr": "Non défini", "de": "Nicht festgelegt",
        "es": "Sin establecer", "fi": "Ei asetettu", "it": "Non impostato",
        "nb_NO": "Ikke angitt",
    },
    "Only a salted hash of the code is stored, so it can be changed but never shown again.": {
        "en": "Only a salted hash of the code is stored, so it can be changed but never shown again.",
        "fr": "Seule une empreinte salée du code est conservée : il peut être changé, jamais réaffiché.",
        "de": "Gespeichert wird nur ein gesalzener Hash des Codes: Er lässt sich ändern, aber nie wieder anzeigen.",
        "es": "Solo se guarda un hash con sal del código, así que se puede cambiar pero nunca volver a mostrar.",
        "fi": "Koodista tallennetaan vain suolattu tiiviste: sen voi vaihtaa muttei nähdä uudelleen.",
        "it": "Del codice si conserva solo un hash con sale: può essere cambiato, mai più mostrato.",
        "nb_NO": "Bare en saltet hash av koden lagres, så den kan endres, men aldri vises igjen.",
    },
    "Stored as a salted hash, so it cannot be shown again — only replaced.": {
        "en": "Stored as a salted hash, so it cannot be shown again — only replaced.",
        "fr": "Conservé sous forme d'empreinte salée : il ne peut plus être affiché, seulement remplacé.",
        "de": "Als gesalzener Hash gespeichert: nicht mehr anzeigbar, nur ersetzbar.",
        "es": "Se guarda como hash con sal: no se puede volver a mostrar, solo sustituir.",
        "fi": "Tallennetaan suolattuna tiivisteenä: sitä ei voi näyttää uudelleen, vain korvata.",
        "it": "Conservato come hash con sale: non può essere mostrato di nuovo, solo sostituito.",
        "nb_NO": "Lagres som en saltet hash, så den kan ikke vises igjen — bare erstattes.",
    },
    "Devices find each other with multicast. Plenty of networks block it — guest Wi-Fi almost always does. When that happens, Scan network on the main page finds them the slow way instead.": {
        "en": "Devices find each other with multicast. Plenty of networks block it — guest Wi-Fi almost always does. When that happens, Scan network on the main page finds them the slow way instead.",
        "fr": "Les appareils se trouvent par multicast. Beaucoup de réseaux le bloquent — le Wi-Fi invité presque toujours. Dans ce cas, Balayer le réseau depuis l'écran principal les trouve par la méthode lente.",
        "de": "Geräte finden einander per Multicast. Viele Netze blockieren das — Gäste-WLAN fast immer. Dann findet Netz absuchen auf der Hauptseite sie stattdessen auf dem langsamen Weg.",
        "es": "Los dispositivos se encuentran por multidifusión. Muchas redes la bloquean, y el wifi de invitados casi siempre. Cuando pasa, Explorar la red desde la pantalla principal los encuentra por la vía lenta.",
        "fi": "Laitteet löytävät toisensa multicastilla. Monet verkot estävät sen — vierasverkko lähes aina. Silloin päänäkymän Etsi verkosta löytää ne hitaammalla tavalla.",
        "it": "I dispositivi si trovano tramite multicast. Molte reti lo bloccano, e il Wi-Fi per ospiti quasi sempre. In quel caso Scansiona la rete dalla schermata principale li trova per la via lenta.",
        "nb_NO": "Enheter finner hverandre med multicast. Mange nettverk blokkerer det — gjeste-wifi nesten alltid. Da finner Søk i nettverket på hovedsiden dem på den langsomme måten i stedet.",
    },
    "Not affiliated with the LocalSend project. Released under the MIT licence.": {
        "en": "Not affiliated with the LocalSend project. Released under the MIT licence.",
        "fr": "Sans lien avec le projet LocalSend. Distribué sous licence MIT.",
        "de": "Nicht mit dem LocalSend-Projekt verbunden. Veröffentlicht unter der MIT-Lizenz.",
        "es": "Sin relación con el proyecto LocalSend. Publicado con licencia MIT.",
        "fi": "Ei yhteydessä LocalSend-projektiin. Julkaistu MIT-lisenssillä.",
        "it": "Non affiliato al progetto LocalSend. Distribuito con licenza MIT.",
        "nb_NO": "Ikke tilknyttet LocalSend-prosjektet. Utgitt under MIT-lisensen.",
    },

    # --- plurals ---------------------------------------------------------------
    "%n file(s)": {
        "en": ["%n file", "%n files"],
        "fr": ["%n fichier", "%n fichiers"],
        "de": ["%n Datei", "%n Dateien"],
        "es": ["%n archivo", "%n archivos"],
        "fi": ["%n tiedosto", "%n tiedostoa"],
        "it": ["%n file", "%n file"],
        "nb_NO": ["%n fil", "%n filer"],
    },
    "%n file(s) ready": {
        "en": ["%n file ready", "%n files ready"],
        "fr": ["%n fichier prêt", "%n fichiers prêts"],
        "de": ["%n Datei bereit", "%n Dateien bereit"],
        "es": ["%n archivo listo", "%n archivos listos"],
        "fi": ["%n tiedosto valmiina", "%n tiedostoa valmiina"],
        "it": ["%n file pronto", "%n file pronti"],
        "nb_NO": ["%n fil klar", "%n filer klare"],
    },
    "%n file(s) received": {
        "en": ["%n file received", "%n files received"],
        "fr": ["%n fichier reçu", "%n fichiers reçus"],
        "de": ["%n Datei empfangen", "%n Dateien empfangen"],
        "es": ["%n archivo recibido", "%n archivos recibidos"],
        "fi": ["%n tiedosto vastaanotettu", "%n tiedostoa vastaanotettu"],
        "it": ["%n file ricevuto", "%n file ricevuti"],
        "nb_NO": ["%n fil mottatt", "%n filer mottatt"],
    },
    "%n file(s) sent": {
        "en": ["%n file sent", "%n files sent"],
        "fr": ["%n fichier envoyé", "%n fichiers envoyés"],
        "de": ["%n Datei gesendet", "%n Dateien gesendet"],
        "es": ["%n archivo enviado", "%n archivos enviados"],
        "fi": ["%n tiedosto lähetetty", "%n tiedostoa lähetetty"],
        "it": ["%n file inviato", "%n file inviati"],
        "nb_NO": ["%n fil sendt", "%n filer sendt"],
    },
    "Saved %n file(s)": {
        "en": ["Saved %n file", "Saved %n files"],
        "fr": ["%n fichier enregistré", "%n fichiers enregistrés"],
        "de": ["%n Datei gespeichert", "%n Dateien gespeichert"],
        "es": ["Se guardó %n archivo", "Se guardaron %n archivos"],
        "fi": ["Tallennettiin %n tiedosto", "Tallennettiin %n tiedostoa"],
        "it": ["Salvato %n file", "Salvati %n file"],
        "nb_NO": ["Lagret %n fil", "Lagret %n filer"],
    },
    "Sent %n file(s)": {
        "en": ["Sent %n file", "Sent %n files"],
        "fr": ["%n fichier envoyé", "%n fichiers envoyés"],
        "de": ["%n Datei gesendet", "%n Dateien gesendet"],
        "es": ["Se envió %n archivo", "Se enviaron %n archivos"],
        "fi": ["Lähetettiin %n tiedosto", "Lähetettiin %n tiedostoa"],
        "it": ["Inviato %n file", "Inviati %n file"],
        "nb_NO": ["Sendte %n fil", "Sendte %n filer"],
    },
    "%n transfer(s)": {
        "en": ["%n transfer", "%n transfers"],
        "fr": ["%n transfert", "%n transferts"],
        "de": ["%n Übertragung", "%n Übertragungen"],
        "es": ["%n transferencia", "%n transferencias"],
        "fi": ["%n siirto", "%n siirtoa"],
        "it": ["%n trasferimento", "%n trasferimenti"],
        "nb_NO": ["%n overføring", "%n overføringer"],
    },
    "%n nearby": {
        "en": ["%n nearby", "%n nearby"],
        "fr": ["%n à proximité", "%n à proximité"],
        "de": ["%n in der Nähe", "%n in der Nähe"],
        "es": ["%n cerca", "%n cerca"],
        "fi": ["%n lähellä", "%n lähellä"],
        "it": ["%n nelle vicinanze", "%n nelle vicinanze"],
        "nb_NO": ["%n i nærheten", "%n i nærheten"],
    },
    "No devices": {
        "en": "No devices", "fr": "Aucun appareil", "de": "Keine Geräte",
        "es": "Sin dispositivos", "fi": "Ei laitteita", "it": "Nessun dispositivo",
        "nb_NO": "Ingen enheter",
    },
}


def strip_comments(text):
    return LINE_COMMENT.sub("", BLOCK_COMMENT.sub("", text))


def unescape_qml(source):
    return (source.replace("\\n", "\n").replace("\\t", "\t")
                  .replace("\\r", "\r").replace('\\"', '"')
                  .replace("\\\\", "\\"))


def collect():
    """context -> sorted list of source strings, read from the QML."""
    contexts = {}
    for path in sorted(QML_DIR.rglob("*.qml")):
        text = strip_comments(path.read_text(encoding="utf-8"))
        sources = sorted(set(unescape_qml(s) for s in QSTR.findall(text)))
        if sources:
            contexts[path.stem] = sources
    return contexts


def render(locale, contexts):
    lines = ['<?xml version="1.0" encoding="utf-8"?>',
             "<!DOCTYPE TS>",
             f'<TS version="2.1" language="{locale}">']

    for context in sorted(contexts):
        lines.append("<context>")
        lines.append(f"    <name>{escape(context)}</name>")

        for source in contexts[context]:
            translation = TRANSLATIONS[source][locale]
            plural = isinstance(translation, list)

            lines.append('    <message numerus="yes">' if plural
                         else "    <message>")
            lines.append(f"        <source>{escape(source)}</source>")

            if plural:
                lines.append("        <translation>")
                for form in translation:
                    lines.append(f"            <numerusform>{escape(form)}"
                                 "</numerusform>")
                lines.append("        </translation>")
            else:
                lines.append(f"        <translation>{escape(translation)}"
                             "</translation>")

            lines.append("    </message>")

        lines.append("</context>")

    lines.append("</TS>")
    lines.append("")
    return "\n".join(lines)


def main():
    contexts = collect()

    wanted = set()
    for sources in contexts.values():
        wanted.update(sources)

    missing = sorted(wanted - set(TRANSLATIONS))
    if missing:
        print("No translation for:")
        for source in missing:
            print(f"    {source}")
        return 1

    unused = sorted(set(TRANSLATIONS) - wanted)
    if unused:
        print("In the table but used by no QML file:")
        for source in unused:
            print(f"    {source}")
        return 1

    for locale in LOCALES:
        for source, forms in TRANSLATIONS.items():
            if locale not in forms:
                print(f"{source!r} has no {locale} translation")
                return 1

    TS_DIR.mkdir(exist_ok=True)
    for locale in LOCALES:
        path = TS_DIR / f"harbour-localsend-{locale}.ts"
        path.write_text(render(locale, contexts), encoding="utf-8")
        print(f"wrote {path.relative_to(ROOT)}")

    print(f"\n{len(wanted)} strings, {len(contexts)} contexts, "
          f"{len(LOCALES)} locales")
    return 0


if __name__ == "__main__":
    sys.exit(main())
