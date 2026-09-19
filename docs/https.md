# HTTPS et installation USB depuis le navigateur

Adresse : **https://192.168.50.75:8443**.

L'ancienne adresse `http://192.168.50.75:6052` redirige vers HTTPS. Caddy
termine la connexion TLS et transmet les requêtes, y compris les WebSockets,
à Device Builder sur `127.0.0.1:6054`.

## Faire reconnaître le certificat sur Ubuntu / Linux

Le certificat est émis par une autorité locale propre à cette installation.
Il faut importer son **certificat public racine**, une seule fois sur chaque
ordinateur/navigateur utilisé. Pour l'USB dans le navigateur, utiliser Chrome,
Chromium ou Edge ; Firefox ne prend pas en charge Web Serial.

1. Télécharger [ESPHome-CA.crt](http://192.168.50.75:6052/esphome-ca.crt).
2. Dans Chrome/Chromium, ouvrir `chrome://settings/certificates` (redirigé
   vers le gestionnaire de certificats selon la version). Dans Edge, utiliser
   `edge://settings/privacy` puis rechercher **Gérer les certificats**.
3. Dans les certificats locaux/personnalisés, ouvrir **Autorités** ou
   **Certificats de confiance**, puis **Importer**. Choisir `ESPHome-CA.crt`
   et autoriser cette autorité à identifier les sites web si demandé.
4. Fermer complètement le navigateur, puis le relancer.
5. Ouvrir **https://192.168.50.75:8443**. La connexion doit être reconnue
   sans avertissement de certificat avant d'essayer le flash USB.

Simplement contourner l'avertissement de certificat n'est pas équivalent à
installer l'autorité de confiance pour les fonctions USB du navigateur.
La carte doit être branchée sur l'ordinateur qui exécute le navigateur.

Empreinte SHA-256 du certificat racine créé sur ce PC le 19 septembre 2026 :

```text
88:E6:90:1B:02:D9:24:22:CF:81:06:57:87:E8:36:58:DC:E9:A0:98:19:F2:D3:7B:06:D5:8A:BE:D6:FC:58:E7
```

Elle est consultable dans les détails du certificat ou avec :

```bash
openssl x509 -in ~/Téléchargements/ESPHome-CA.crt -noout -fingerprint -sha256
```

Pour les outils système Ubuntu (par exemple `curl`), l'autorité peut également
être installée dans le magasin système :

```bash
sudo install -m 644 ~/Téléchargements/ESPHome-CA.crt /usr/local/share/ca-certificates/esphome-local-ca.crt
sudo update-ca-certificates
```

Le magasin système et celui du navigateur peuvent être distincts : vérifier
la reconnaissance du certificat dans le navigateur utilisé pour flasher.

## Installation et restauration du proxy

Le binaire Caddy provient du paquet Ubuntu et est extrait dans
`~/.local/share/esphome-caddy`. Aucun service Caddy système n'est installé.
Les ports `8443` et `6052` permettent son exécution sans privilèges administrateur.

Après restauration du dépôt et de l'environnement ESPHome :

```bash
bash ~/esphome/scripts/install-https.sh
systemctl --user enable --now ~/esphome/systemd/esphome-dashboard.service
systemctl --user enable --now ~/esphome/systemd/esphome-https.service
```

Adapter l'adresse IP dans `Caddyfile` si le PC change d'adresse, puis redémarrer
`esphome-https`. Une réservation DHCP dans le routeur permet de conserver
`192.168.50.75`.

## Emplacements et certificats

- `Caddyfile` : adresses, certificats internes, redirection et proxy.
- `systemd/esphome-https.service` : service utilisateur démarré automatiquement.
- `~/.local/share/esphome-https/caddy/` : certificats et clés privées générés
  et renouvelés automatiquement par Caddy ; hors du dépôt Git.
- `~/.local/share/esphome-https/caddy/pki/authorities/local/root.crt` :
  certificat racine public distribué aux navigateurs.

La route `/esphome-ca.crt` expose uniquement `root.crt`. Les clés privées
restent sur le serveur. Conserver le dossier de données Caddy dans une
sauvegarde privée pour retrouver la même autorité après restauration.
Sinon Caddy en générera une nouvelle et il faudra réimporter son certificat
sur les clients ; l'empreinte ci-dessus sera alors à mettre à jour.

## Commandes utiles

```bash
systemctl --user status esphome-https
systemctl --user restart esphome-https
journalctl --user -u esphome-https -n 30 --no-pager
```

Vérifier TLS depuis le serveur sans modifier son magasin de certificats :

```bash
curl --cacert ~/.local/share/esphome-https/caddy/pki/authorities/local/root.crt https://192.168.50.75:8443/
```

Pour mettre Caddy à jour depuis le paquet actuellement disponible dans les
dépôts Ubuntu :

```bash
systemctl --user stop esphome-https
bash ~/esphome/scripts/install-https.sh
systemctl --user start esphome-https
```
