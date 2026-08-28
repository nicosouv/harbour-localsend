<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr">
<context>
    <name>AboutPage</name>
    <message>
        <source>About</source>
        <translation>À propos</translation>
    </message>
    <message>
        <source>An unofficial LocalSend client for Sailfish OS. Files go straight from one device to the other over your own network — no account, no server, no Internet connection needed.</source>
        <translation>Un client LocalSend non officiel pour Sailfish OS. Les fichiers passent directement d'un appareil à l'autre sur votre propre réseau — sans compte, sans serveur, sans connexion Internet.</translation>
    </message>
    <message>
        <source>Devices find each other with multicast. Plenty of networks block it — guest Wi-Fi almost always does. When that happens, Scan network on the main page finds them the slow way instead.</source>
        <translation>Les appareils se trouvent par multicast. Beaucoup de réseaux le bloquent — le Wi-Fi invité presque toujours. Dans ce cas, Balayer le réseau depuis l'écran principal les trouve par la méthode lente.</translation>
    </message>
    <message>
        <source>Encryption is off, so transfers use plain HTTP on port %1 and are readable by anyone who can watch the network. On a home network or your own hotspot that is nobody; on café or office Wi-Fi it may not be.</source>
        <translation>Le chiffrement est désactivé : les transferts passent en HTTP en clair sur le port %1 et sont lisibles par quiconque peut observer le réseau. Chez vous ou sur votre partage de connexion, cela ne concerne personne ; sur le Wi-Fi d'un café ou d'un bureau, peut-être si.</translation>
    </message>
    <message>
        <source>Fingerprint</source>
        <translation>Empreinte</translation>
    </message>
    <message>
        <source>Good to know</source>
        <translation>Bon à savoir</translation>
    </message>
    <message>
        <source>HTTP (not encrypted)</source>
        <translation>HTTP (non chiffré)</translation>
    </message>
    <message>
        <source>HTTPS (encrypted)</source>
        <translation>HTTPS (chiffré)</translation>
    </message>
    <message>
        <source>Links</source>
        <translation>Liens</translation>
    </message>
    <message>
        <source>LocalSend v%1</source>
        <translation>LocalSend v%1</translation>
    </message>
    <message>
        <source>Not affiliated with the LocalSend project. Released under the MIT licence.</source>
        <translation>Sans lien avec le projet LocalSend. Distribué sous licence MIT.</translation>
    </message>
    <message>
        <source>Protocol</source>
        <translation>Protocole</translation>
    </message>
    <message>
        <source>Source and issues</source>
        <translation>Code source et signalements</translation>
    </message>
    <message>
        <source>The LocalSend project</source>
        <translation>Le projet LocalSend</translation>
    </message>
    <message>
        <source>This device</source>
        <translation>Cet appareil</translation>
    </message>
    <message>
        <source>Transfers are encrypted between the two devices with a certificate this phone generated for itself. There is no certificate authority on a local network, so what identifies a device is the fingerprint above: it travels in every announcement, and a device presenting anything else is refused before a single byte is sent.</source>
        <translation>Les transferts sont chiffrés entre les deux appareils avec un certificat que ce téléphone a généré lui-même. Il n'existe pas d'autorité de certification sur un réseau local : ce qui identifie un appareil, c'est l'empreinte ci-dessus. Elle voyage dans chaque annonce, et un appareil qui en présente une autre est refusé avant le moindre octet.</translation>
    </message>
    <message>
        <source>Transport</source>
        <translation>Transport</translation>
    </message>
    <message>
        <source>Version</source>
        <translation>Version</translation>
    </message>
</context>
<context>
    <name>AddDevicePage</name>
    <message>
        <source>Add by address</source>
        <translation>Ajouter par adresse</translation>
    </message>
    <message>
        <source>Address</source>
        <translation>Adresse</translation>
    </message>
    <message>
        <source>For a device on another network, behind a VPN, or on a Wi-Fi that keeps clients apart. Both plain and encrypted transports are tried.</source>
        <translation>Pour un appareil sur un autre réseau, derrière un VPN, ou sur un Wi-Fi qui isole les clients entre eux. Les transports en clair et chiffré sont essayés tous les deux.</translation>
    </message>
    <message>
        <source>Forget</source>
        <translation>Oublier</translation>
    </message>
    <message>
        <source>Forgetting</source>
        <translation>Oubli</translation>
    </message>
    <message>
        <source>Found %1</source>
        <translation>%1 trouvé</translation>
    </message>
    <message>
        <source>Leave this alone unless the other device was moved off the standard port.</source>
        <translation>À ne changer que si l'autre appareil a quitté le port standard.</translation>
    </message>
    <message>
        <source>Look for it</source>
        <translation>Le chercher</translation>
    </message>
    <message>
        <source>Nothing answered at that address</source>
        <translation>Rien n'a répondu à cette adresse</translation>
    </message>
    <message>
        <source>Port</source>
        <translation>Port</translation>
    </message>
    <message>
        <source>Remembered addresses</source>
        <translation>Adresses mémorisées</translation>
    </message>
</context>
<context>
    <name>CoverPage</name>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n fichier</numerusform>
            <numerusform>%n fichiers</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n nearby</source>
        <translation>
            <numerusform>%n à proximité</numerusform>
            <numerusform>%n à proximité</numerusform>
        </translation>
    </message>
    <message>
        <source>Incoming</source>
        <translation>Entrant</translation>
    </message>
    <message>
        <source>No devices</source>
        <translation>Aucun appareil</translation>
    </message>
    <message>
        <source>Receiving</source>
        <translation>Réception</translation>
    </message>
    <message>
        <source>Receiving off</source>
        <translation>Réception désactivée</translation>
    </message>
    <message>
        <source>Sending</source>
        <translation>Envoi</translation>
    </message>
</context>
<context>
    <name>DeviceDelegate</name>
    <message>
        <source>Choose files to send</source>
        <translation>Choisir les fichiers à envoyer</translation>
    </message>
    <message>
        <source>Device details</source>
        <translation>Détails de l'appareil</translation>
    </message>
    <message>
        <source>Send staged files</source>
        <translation>Envoyer les fichiers en attente</translation>
    </message>
</context>
<context>
    <name>HistoryPage</name>
    <message numerus="yes">
        <source>%n transfer(s)</source>
        <translation>
            <numerusform>%n transfert</numerusform>
            <numerusform>%n transferts</numerusform>
        </translation>
    </message>
    <message>
        <source>+%1 more</source>
        <translation>+%1 autre(s)</translation>
    </message>
    <message>
        <source>Clear history</source>
        <translation>Effacer l'historique</translation>
    </message>
    <message>
        <source>Clearing history</source>
        <translation>Effacement de l'historique</translation>
    </message>
    <message>
        <source>From %1</source>
        <translation>De %1</translation>
    </message>
    <message>
        <source>History</source>
        <translation>Historique</translation>
    </message>
    <message>
        <source>Nothing yet</source>
        <translation>Rien pour l'instant</translation>
    </message>
    <message>
        <source>Open folder</source>
        <translation>Ouvrir le dossier</translation>
    </message>
    <message>
        <source>Remove</source>
        <translation>Retirer</translation>
    </message>
    <message>
        <source>Removing</source>
        <translation>Retrait</translation>
    </message>
    <message>
        <source>To %1</source>
        <translation>Vers %1</translation>
    </message>
    <message>
        <source>Transfers you send and receive will be listed here.</source>
        <translation>Les transferts envoyés et reçus apparaîtront ici.</translation>
    </message>
    <message>
        <source>incomplete</source>
        <translation>incomplet</translation>
    </message>
</context>
<context>
    <name>MainPage</name>
    <message>
        <source>About</source>
        <translation>À propos</translation>
    </message>
    <message>
        <source>Add by address</source>
        <translation>Ajouter par adresse</translation>
    </message>
    <message>
        <source>Add files</source>
        <translation>Ajouter des fichiers</translation>
    </message>
    <message>
        <source>Address</source>
        <translation>Adresse</translation>
    </message>
    <message>
        <source>Clear selection</source>
        <translation>Vider la sélection</translation>
    </message>
    <message>
        <source>Device</source>
        <translation>Appareil</translation>
    </message>
    <message>
        <source>Device name</source>
        <translation>Nom de l'appareil</translation>
    </message>
    <message>
        <source>Encrypted</source>
        <translation>Chiffré</translation>
    </message>
    <message>
        <source>Fingerprint</source>
        <translation>Empreinte</translation>
    </message>
    <message>
        <source>History</source>
        <translation>Historique</translation>
    </message>
    <message>
        <source>LocalSend</source>
        <translation>LocalSend</translation>
    </message>
    <message>
        <source>Look again</source>
        <translation>Chercher à nouveau</translation>
    </message>
    <message>
        <source>Model</source>
        <translation>Modèle</translation>
    </message>
    <message>
        <source>Nearby devices</source>
        <translation>Appareils à proximité</translation>
    </message>
    <message>
        <source>Nobody yet</source>
        <translation>Personne pour l'instant</translation>
    </message>
    <message>
        <source>Not encrypted</source>
        <translation>Non chiffré</translation>
    </message>
    <message>
        <source>Not listening</source>
        <translation>Pas à l'écoute</translation>
    </message>
    <message>
        <source>Open LocalSend on another device on the same network. It should turn up here within a few seconds. If it does not, pull down: Search every address goes through the whole subnet, and Add by address reaches one that is somewhere else entirely.</source>
        <translation>Ouvrez LocalSend sur un autre appareil du même réseau. Il devrait apparaître ici en quelques secondes. Sinon, tirez vers le bas : Sonder toutes les adresses parcourt tout le sous-réseau, et Ajouter par adresse atteint un appareil situé ailleurs.</translation>
    </message>
    <message>
        <source>Port %1 is unavailable</source>
        <translation>Le port %1 est indisponible</translation>
    </message>
    <message>
        <source>Ready on port %1</source>
        <translation>Prêt sur le port %1</translation>
    </message>
    <message>
        <source>Ready · %1 on port %2</source>
        <translation>Prêt · %1 sur le port %2</translation>
    </message>
    <message>
        <source>Receiving is off — others cannot send to you</source>
        <translation>Réception désactivée — personne ne peut vous envoyer de fichiers</translation>
    </message>
    <message>
        <source>Receiving is off. Turn it back on in Settings to be found.</source>
        <translation>La réception est désactivée. Réactivez-la dans les réglages pour être trouvé.</translation>
    </message>
    <message>
        <source>Scanning the network… %1%</source>
        <translation>Balayage du réseau… %1 %</translation>
    </message>
    <message>
        <source>Search every address</source>
        <translation>Sonder toutes les adresses</translation>
    </message>
    <message>
        <source>Select files to send</source>
        <translation>Sélectionner les fichiers à envoyer</translation>
    </message>
    <message>
        <source>Send files</source>
        <translation>Envoyer des fichiers</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Réglages</translation>
    </message>
    <message>
        <source>Shown to other devices</source>
        <translation>Visible par les autres appareils</translation>
    </message>
    <message>
        <source>Stop searching</source>
        <translation>Arrêter la recherche</translation>
    </message>
    <message>
        <source>Suggest another</source>
        <translation>En proposer un autre</translation>
    </message>
    <message>
        <source>This network is blocking discovery. Pull down and choose Scan network.</source>
        <translation>Ce réseau bloque la découverte. Tirez vers le bas et choisissez Balayer le réseau.</translation>
    </message>
    <message>
        <source>Transport</source>
        <translation>Transport</translation>
    </message>
    <message>
        <source>Type</source>
        <translation>Type</translation>
    </message>
    <message>
        <source>Unknown</source>
        <translation>Inconnu</translation>
    </message>
    <message>
        <source>Your device name</source>
        <translation>Le nom de votre appareil</translation>
    </message>
</context>
<context>
    <name>PinDialog</name>
    <message>
        <source>%1 is asking for a PIN before accepting files.</source>
        <translation>%1 demande un code PIN avant d'accepter des fichiers.</translation>
    </message>
    <message>
        <source>PIN</source>
        <translation>Code PIN</translation>
    </message>
    <message>
        <source>PIN required</source>
        <translation>Code PIN requis</translation>
    </message>
    <message>
        <source>Send</source>
        <translation>Envoyer</translation>
    </message>
    <message>
        <source>That code was not accepted. Try again.</source>
        <translation>Ce code a été refusé. Réessayez.</translation>
    </message>
</context>
<context>
    <name>ReceiveRequestPage</name>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n fichier</numerusform>
            <numerusform>%n fichiers</numerusform>
        </translation>
    </message>
    <message>
        <source>Accept</source>
        <translation>Accepter</translation>
    </message>
    <message>
        <source>Decline</source>
        <translation>Refuser</translation>
    </message>
    <message>
        <source>Incoming files</source>
        <translation>Fichiers entrants</translation>
    </message>
    <message>
        <source>Saved to %1</source>
        <translation>Enregistré dans %1</translation>
    </message>
    <message>
        <source>What they are sending</source>
        <translation>Ce qui vous est envoyé</translation>
    </message>
    <message>
        <source>from %1</source>
        <translation>depuis %1</translation>
    </message>
</context>
<context>
    <name>SelectionPage</name>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n fichier</numerusform>
            <numerusform>%n fichiers</numerusform>
        </translation>
    </message>
    <message>
        <source>Add more files</source>
        <translation>Ajouter d'autres fichiers</translation>
    </message>
    <message>
        <source>Clear all</source>
        <translation>Tout vider</translation>
    </message>
    <message>
        <source>Nothing staged</source>
        <translation>Rien en attente</translation>
    </message>
    <message>
        <source>Pull down to add files</source>
        <translation>Tirez vers le bas pour ajouter des fichiers</translation>
    </message>
    <message>
        <source>Ready to send</source>
        <translation>Prêt à envoyer</translation>
    </message>
    <message>
        <source>Remove</source>
        <translation>Retirer</translation>
    </message>
    <message>
        <source>Removing</source>
        <translation>Retrait</translation>
    </message>
    <message>
        <source>Select files to send</source>
        <translation>Sélectionner les fichiers à envoyer</translation>
    </message>
</context>
<context>
    <name>SelectionTray</name>
    <message numerus="yes">
        <source>%n file(s) ready</source>
        <translation>
            <numerusform>%n fichier prêt</numerusform>
            <numerusform>%n fichiers prêts</numerusform>
        </translation>
    </message>
    <message>
        <source>pick a device to send</source>
        <translation>choisissez un appareil</translation>
    </message>
</context>
<context>
    <name>SettingsPage</name>
    <message>
        <source>4 to 8 digits</source>
        <translation>4 à 8 chiffres</translation>
    </message>
    <message>
        <source>53317 is the standard. Change it only if something else is using the port.</source>
        <translation>53317 est le port standard. Ne le changez que si un autre programme l'utilise.</translation>
    </message>
    <message>
        <source>A folder per sender</source>
        <translation>Un dossier par expéditeur</translation>
    </message>
    <message>
        <source>A notification when a transfer arrives or finishes in the background.</source>
        <translation>Une notification lorsqu'un transfert arrive ou se termine en arrière-plan.</translation>
    </message>
    <message>
        <source>Accept without asking</source>
        <translation>Accepter sans demander</translation>
    </message>
    <message>
        <source>Allow incoming files</source>
        <translation>Autoriser les fichiers entrants</translation>
    </message>
    <message>
        <source>Change the PIN</source>
        <translation>Changer le code PIN</translation>
    </message>
    <message>
        <source>Device name</source>
        <translation>Nom de l'appareil</translation>
    </message>
    <message>
        <source>Encrypt transfers</source>
        <translation>Chiffrer les transferts</translation>
    </message>
    <message>
        <source>Files are encrypted between the two devices. Turning this off makes every transfer readable by anyone on the same network.</source>
        <translation>Les fichiers sont chiffrés entre les deux appareils. Désactiver rend chaque transfert lisible par n'importe qui sur le même réseau.</translation>
    </message>
    <message>
        <source>Files are saved as soon as they arrive. Convenient at home, unwise on a network you share.</source>
        <translation>Les fichiers sont enregistrés dès leur arrivée. Pratique chez soi, imprudent sur un réseau partagé.</translation>
    </message>
    <message>
        <source>Interface language</source>
        <translation>Langue de l'interface</translation>
    </message>
    <message>
        <source>Keep a history</source>
        <translation>Conserver un historique</translation>
    </message>
    <message>
        <source>Keep going with the screen off</source>
        <translation>Continuer écran éteint</translation>
    </message>
    <message>
        <source>Language</source>
        <translation>Langue</translation>
    </message>
    <message>
        <source>Listening port</source>
        <translation>Port d'écoute</translation>
    </message>
    <message>
        <source>Not set</source>
        <translation>Non défini</translation>
    </message>
    <message>
        <source>Notify me</source>
        <translation>Me notifier</translation>
    </message>
    <message>
        <source>Only a salted hash of the code is stored, so it can be changed but never shown again.</source>
        <translation>Seule une empreinte salée du code est conservée : il peut être changé, jamais réaffiché.</translation>
    </message>
    <message>
        <source>Other LocalSend devices look on 53317 by default. A different port still works, but only if the other side is told about it.</source>
        <translation>Les autres appareils LocalSend cherchent sur 53317 par défaut. Un autre port fonctionne, mais seulement si l'autre côté en est informé.</translation>
    </message>
    <message>
        <source>PIN</source>
        <translation>Code PIN</translation>
    </message>
    <message>
        <source>Port</source>
        <translation>Port</translation>
    </message>
    <message>
        <source>Received files go into a subfolder named after the device that sent them.</source>
        <translation>Les fichiers reçus vont dans un sous-dossier au nom de l'appareil expéditeur.</translation>
    </message>
    <message>
        <source>Receiving</source>
        <translation>Réception</translation>
    </message>
    <message>
        <source>Records what was sent and received, and where it was saved.</source>
        <translation>Note ce qui a été envoyé et reçu, et où cela a été enregistré.</translation>
    </message>
    <message>
        <source>Remove the PIN</source>
        <translation>Supprimer le code PIN</translation>
    </message>
    <message>
        <source>Require a PIN</source>
        <translation>Exiger un code PIN</translation>
    </message>
    <message>
        <source>Save to</source>
        <translation>Enregistrer dans</translation>
    </message>
    <message>
        <source>Saving</source>
        <translation>Enregistrement</translation>
    </message>
    <message>
        <source>Security</source>
        <translation>Sécurité</translation>
    </message>
    <message>
        <source>Senders must enter this code before you are even asked.</source>
        <translation>L'expéditeur doit saisir ce code avant même que la question vous soit posée.</translation>
    </message>
    <message>
        <source>Set</source>
        <translation>Défini</translation>
    </message>
    <message>
        <source>Set a PIN</source>
        <translation>Définir un code PIN</translation>
    </message>
    <message>
        <source>Settings</source>
        <translation>Réglages</translation>
    </message>
    <message>
        <source>Stops the device suspending mid-transfer. Uses more battery.</source>
        <translation>Empêche la mise en veille en plein transfert. Consomme plus de batterie.</translation>
    </message>
    <message>
        <source>Stored as a salted hash, so it cannot be shown again — only replaced.</source>
        <translation>Conservé sous forme d'empreinte salée : il ne peut plus être affiché, seulement remplacé.</translation>
    </message>
    <message>
        <source>Suggest another name</source>
        <translation>Proposer un autre nom</translation>
    </message>
    <message>
        <source>The app reloads when this changes.</source>
        <translation>L'application se recharge lors du changement.</translation>
    </message>
    <message>
        <source>This device</source>
        <translation>Cet appareil</translation>
    </message>
    <message>
        <source>This device is identified by the fingerprint of its certificate, so changing this setting makes it look like a new device to everyone else.</source>
        <translation>Cet appareil est identifié par l'empreinte de son certificat : changer ce réglage le fait apparaître comme un nouvel appareil pour tous les autres.</translation>
    </message>
    <message>
        <source>Unavailable on this device: %1</source>
        <translation>Indisponible sur cet appareil : %1</translation>
    </message>
    <message>
        <source>What other devices call you.</source>
        <translation>Le nom sous lequel les autres appareils vous voient.</translation>
    </message>
    <message>
        <source>When off, this device stops announcing itself and refuses transfers.</source>
        <translation>Désactivé, cet appareil cesse de s'annoncer et refuse les transferts.</translation>
    </message>
    <message>
        <source>Where to save incoming files</source>
        <translation>Où enregistrer les fichiers reçus</translation>
    </message>
    <message>
        <source>While transferring</source>
        <translation>Pendant les transferts</translation>
    </message>
</context>
<context>
    <name>TransferFileDelegate</name>
    <message>
        <source>failed</source>
        <translation>échec</translation>
    </message>
    <message>
        <source>skipped</source>
        <translation>ignoré</translation>
    </message>
</context>
<context>
    <name>TransferPage</name>
    <message>
        <source>%1 · %2 left</source>
        <translation>%1 · %2 restant</translation>
    </message>
    <message>
        <source>Done</source>
        <translation>Terminé</translation>
    </message>
    <message>
        <source>Files</source>
        <translation>Fichiers</translation>
    </message>
    <message>
        <source>Open</source>
        <translation>Ouvrir</translation>
    </message>
    <message>
        <source>Receiving</source>
        <translation>Réception</translation>
    </message>
    <message numerus="yes">
        <source>Saved %n file(s)</source>
        <translation>
            <numerusform>%n fichier enregistré</numerusform>
            <numerusform>%n fichiers enregistrés</numerusform>
        </translation>
    </message>
    <message>
        <source>Sending</source>
        <translation>Envoi</translation>
    </message>
    <message numerus="yes">
        <source>Sent %n file(s)</source>
        <translation>
            <numerusform>%n fichier envoyé</numerusform>
            <numerusform>%n fichiers envoyés</numerusform>
        </translation>
    </message>
    <message>
        <source>Starting…</source>
        <translation>Démarrage…</translation>
    </message>
    <message>
        <source>Stop</source>
        <translation>Arrêter</translation>
    </message>
    <message>
        <source>Transfer failed</source>
        <translation>Échec du transfert</translation>
    </message>
    <message>
        <source>Transfer stopped</source>
        <translation>Transfert arrêté</translation>
    </message>
    <message>
        <source>Waiting for %1 to accept</source>
        <translation>En attente de l'accord de %1</translation>
    </message>
    <message>
        <source>in %1</source>
        <translation>dans %1</translation>
    </message>
</context>
<context>
    <name>harbour-localsend</name>
    <message>
        <source>%1 wants to send you files</source>
        <translation>%1 veut vous envoyer des fichiers</translation>
    </message>
    <message numerus="yes">
        <source>%n file(s)</source>
        <translation>
            <numerusform>%n fichier</numerusform>
            <numerusform>%n fichiers</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n file(s) received</source>
        <translation>
            <numerusform>%n fichier reçu</numerusform>
            <numerusform>%n fichiers reçus</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <source>%n file(s) sent</source>
        <translation>
            <numerusform>%n fichier envoyé</numerusform>
            <numerusform>%n fichiers envoyés</numerusform>
        </translation>
    </message>
    <message>
        <source>Transfer incomplete</source>
        <translation>Transfert incomplet</translation>
    </message>
</context>
</TS>
