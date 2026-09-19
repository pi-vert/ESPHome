# Environnement de développement ESPHome

Configurations des appareils, packages partagés et documentation, prêts à être
versionnés sur GitHub. Versions de référence : **ESPHome 2026.9.0** et
**ESPHome Device Builder 1.14.9**, définies dans `requirements.txt`.

## Interface web

Depuis un appareil du réseau local, ouvrir **http://192.168.50.75:6052**.
Sur cette machine, **http://localhost:6052** fonctionne également.

L'interface fonctionne en arrière-plan via le service utilisateur
`esphome-dashboard.service`. Son démarrage automatique est activé ; le mode
« linger » de l'utilisateur permet son démarrage même sans ouverture de session.

```bash
systemctl --user status esphome-dashboard
systemctl --user restart esphome-dashboard
systemctl --user stop esphome-dashboard
journalctl --user -u esphome-dashboard -f
```

L'interface écoute sur toutes les interfaces IPv4 (`0.0.0.0`), port `6052`.
L'adresse actuelle du PC est `192.168.50.75` ; si elle change, utiliser sa
nouvelle adresse dans `http://ADRESSE_IP_DU_PC:6052`.

## Arborescence

```text
esphome/
├── *.yaml                     # Un fichier par appareil (à créer)
├── esp32.yaml.example         # Modèle pour ESP32 classique
├── secrets.yaml.example       # Liste des secrets à renseigner
├── secrets.yaml               # Valeurs privées locales, ignorées par Git
├── packages/
│   └── base.yaml              # Wi-Fi, API, OTA et journaux partagés
├── docs/                      # Fiches appareils et câblage
├── scripts/github-backup.py   # Sauvegarde automatique GitHub
├── systemd/
│   ├── esphome-dashboard.service
│   ├── esphome-github-backup.service
│   └── esphome-github-backup.timer
├── requirements.txt           # Versions des deux outils principaux
├── .gitignore
├── .esphome/                  # Cache et compilation, ignorés par Git
└── backups/                   # Sauvegardes de flash locales, ignorées par Git
```

Les configurations des appareils restent **à la racine** : c'est le dossier
servi par Device Builder. Les fichiers `.example` sont des modèles, pas des
appareils actifs.

## Créer un appareil

Utiliser l'assistant de l'interface web et sélectionner la référence exacte de
la carte, ou partir du modèle fourni :

```bash
cd ~/esphome
cp -n esp32.yaml.example mon-esp32.yaml
cp -n secrets.yaml.example secrets.yaml
```

`cp -n` préserve les fichiers déjà présents. Compléter `secrets.yaml` avec les
clés du modèle qui manquent, puis renseigner les valeurs réelles. Ce fichier
existe déjà sur la machine actuelle. Générer la clé API avec :

```bash
openssl rand -base64 32
```

Adapter `device_name`, `friendly_name` et `esp32.board` dans `mon-esp32.yaml`.
Le modèle utilise `esp32dev` pour un ESP32 classique ; pour une autre variante
(C3, S3…), choisir la carte correspondante avant de compiler.

Employer `!secret nom_de_la_cle` pour tous les identifiants dans les YAML,
y compris ceux créés par l'assistant web : `.gitignore` ne masque pas les
mots de passe écrits directement dans un fichier d'appareil.

## Vérifier, compiler et flasher

Sur cette machine, la commande `esphome` est disponible dans `~/.local/bin` :

```bash
esphome config ~/esphome/mon-esp32.yaml
esphome compile ~/esphome/mon-esp32.yaml
esphome run ~/esphome/mon-esp32.yaml
```

`config` valide le YAML, `compile` construit le firmware et `run` compile,
téléverse puis ouvre les journaux. Ces opérations sont aussi accessibles dans
l'interface web.

Pour le premier flash, brancher la carte avec un câble USB de données et choisir
le port série approprié. L'utilisateur `eric` appartient déjà au groupe
`dialout`. Sur une autre installation Ubuntu :

```bash
sudo usermod -aG dialout "$USER"
```

Se déconnecter puis se reconnecter après l'ajout au groupe. Les compilateurs
sont téléchargés lors de la première compilation. Les mises à jour suivantes
peuvent utiliser le Wi-Fi et OTA.

## Recréer l'environnement sur Ubuntu

Cloner ce dépôt dans `~/esphome`. Installer Python 3.12 et son module `venv`
(Ubuntu 24.04 convient), puis créer un environnement Python dédié :

```bash
sudo apt install python3-venv git
python3 -m venv ~/.local/share/esphome
~/.local/share/esphome/bin/python -m pip install -r ~/esphome/requirements.txt
source ~/.local/share/esphome/bin/activate
```

La machine actuelle utilise déjà un environnement Python 3.12 dédié dans
`~/.local/share/esphome`, avec un interpréteur géré par uv. Les outils ESP-IDF
sont mis en cache dans `~/.cache/esphome/idf`.

Restaurer `secrets.yaml` depuis une sauvegarde privée ou le créer à partir du
modèle, puis activer l'interface :

```bash
systemctl --user enable --now ~/esphome/systemd/esphome-dashboard.service
sudo loginctl enable-linger "$USER"
```

Le service suppose que le dépôt est dans `~/esphome` et que l'environnement
Python est dans `~/.local/share/esphome`. Adapter le fichier du service si ces
emplacements changent. Pour un lancement manuel au premier plan :

```bash
systemctl --user stop esphome-dashboard
~/.local/share/esphome/bin/esphome-device-builder ~/esphome --host 0.0.0.0 --remote-build-host 127.0.0.1
```

Ctrl+C arrête ce lancement manuel ; `systemctl --user start esphome-dashboard`
réactive le service.

### Mises à jour

Modifier les versions dans `requirements.txt`, puis appliquer et vérifier :

```bash
systemctl --user stop esphome-dashboard
~/.local/share/esphome/bin/python -m pip install -r ~/esphome/requirements.txt
~/.local/share/esphome/bin/python -m pip check
systemctl --user start esphome-dashboard
```

Valider et compiler les configurations après une mise à jour. Les versions des
outils principaux sont fixées ; les dépendances transitives et les chaînes de
compilation restent gérées par pip et ESPHome.

## Sauvegarder sur GitHub

Le dépôt distant est **[pi-vert/ESPHome](https://github.com/pi-vert/ESPHome)**,
sur la branche `master`. Le service `esphome-github-backup` sauvegarde les
sources **chaque heure** : il complète les commits locaux de Device Builder,
puis les envoie par SSH sur GitHub.

L'accès nécessite que la clé SSH publique du PC soit autorisée en écriture
dans les clés de déploiement du dépôt. Les instructions de configuration et de
restauration sont dans **[docs/github-backup.md](docs/github-backup.md)**.

Lancer une sauvegarde immédiatement :

```bash
systemctl --user start esphome-github-backup.service
journalctl --user -u esphome-github-backup.service -n 30 --no-pager
```

Le script vérifie les champs d'identifiants usuels des YAML et de leur historique
avant l'envoi. Si l'assistant web a enregistré un identifiant en clair dans un
commit, le journal signale le blocage : déplacer la valeur dans `secrets.yaml`
et nettoyer aussi l'historique concerné avant de réessayer.

Les secrets, sauvegardes de flash, firmwares compilés, caches et réglages locaux
de Device Builder sont exclus de Git. Conserver **une sauvegarde privée séparée
de `secrets.yaml` et de `backups/`**, nécessaire pour une restauration complète.
Le dossier `backups/` contient une sauvegarde binaire d'ESP32-C3 et une copie
privée de l'historique Git avant nettoyage. La sauvegarde binaire ne contient
pas le YAML source d'origine ; la configuration actuelle est `espgesture.yaml`.
