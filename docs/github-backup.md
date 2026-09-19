# Sauvegarde automatique GitHub

Dépôt : https://github.com/pi-vert/ESPHome — branche `master`.

Le timer `esphome-github-backup.timer` exécute la sauvegarde chaque heure,
avec un décalage de zéro à deux minutes. `Persistent=true` rattrape une
exécution manquée au redémarrage du PC. Le mode linger permet l'exécution
sans session ouverte.

## Fonctionnement

Device Builder enregistre déjà les modifications des configurations dans son
historique Git local. Le script `scripts/github-backup.py` complète cet
historique avec les sources modifiées ou ajoutées, puis envoie la branche
`master` vers GitHub.

Les sources prises en compte sont les YAML, modèles `.yaml.example`, Markdown,
`.gitignore` et `requirements.txt` à la racine, ainsi que les fichiers texte
usuels dans `packages/`, `docs/`, `systemd/` et `scripts/`. Les suppressions
de fichiers suivis sont également enregistrées. Les commits automatiques
utilisent l'identité `ESPHome Backup <esphome-backup@localhost>` sans modifier
la configuration Git personnelle.

Les exclusions `.gitignore` restent appliquées. Un contrôle supplémentaire
refuse les fichiers privés dans l'historique et les champs YAML usuels
`password`, `username`, `key`, `token`, `api_key` renseignés en clair plutôt
qu'avec `!secret`. Il contrôle aussi les anciens commits, avant chaque envoi.
Ce contrôle cible ces champs usuels : conserver tous les autres identifiants
éventuels dans `secrets.yaml` également.

Les fichiers déjà indexés manuellement ou une opération Git en cours suspendent
la sauvegarde pour laisser terminer le travail manuel. Une erreur réseau ou SSH
laisse les commits locaux en place ; l'envoi sera retenté à l'heure suivante.
Un historique divergent sur GitHub provoque un échec à résoudre manuellement.

## Authentification SSH

La clé publique du PC est dans `~/.ssh/id_ed25519.pub`. L'ajouter dans les
[clés de déploiement du dépôt](https://github.com/pi-vert/ESPHome/settings/keys),
avec **Allow write access**. La clé privée reste uniquement sur le PC.

Le service utilise SSH en mode non interactif. La clé doit être utilisable sans
saisie dans ce contexte. Vérifier l'accès :

```bash
ssh -o BatchMode=yes -T git@github.com
```

GitHub renvoie un message « successfully authenticated » et un code de sortie
1 même lorsque l'authentification réussit (pas d'accès shell).

## Installation après clonage

Installer d'abord l'environnement Python décrit dans le README. Ce script
utilise PyYAML, déjà installé comme dépendance d'ESPHome.

```bash
git -C ~/esphome remote set-url origin git@github.com:pi-vert/ESPHome.git
systemctl --user link ~/esphome/systemd/esphome-github-backup.service
systemctl --user enable --now ~/esphome/systemd/esphome-github-backup.timer
```

Si le dépôt local ne possède pas encore `origin`, utiliser `remote add` au lieu
de `remote set-url`. La branche locale doit être `master`, celle sauvegardée
par le script.

## Commandes utiles

Vérifier l'historique et les YAML sans rien modifier ni envoyer :

```bash
~/.local/share/esphome/bin/python ~/esphome/scripts/github-backup.py --check
```

Déclencher un envoi immédiat et consulter son résultat :

```bash
systemctl --user start esphome-github-backup.service
journalctl --user -u esphome-github-backup.service -n 30 --no-pager
systemctl --user list-timers esphome-github-backup.timer
```

Suspendre / réactiver l'automatisation :

```bash
systemctl --user disable --now esphome-github-backup.timer
systemctl --user enable --now esphome-github-backup.timer
```

## Historique initial et secrets

Avant le premier envoi, les identifiants d'`espgesture.yaml` ont été déplacés
dans `secrets.yaml` et leurs anciennes occurrences retirées des commits locaux.
Leurs valeurs effectives sont conservées : la configuration de l'appareil
continue à les lire via `!secret`.

Une copie privée de l'historique antérieur est conservée dans
`backups/history-before-github-20260919.bundle`, exclue de Git. Elle contient
les anciennes valeurs en clair ; la conserver avec les sauvegardes privées.

GitHub sauvegarde les sources. Pour une restauration complète, conserver
séparément `secrets.yaml` et le contenu de `backups/`.
