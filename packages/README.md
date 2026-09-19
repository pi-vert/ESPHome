# Packages partagés

Les fichiers YAML de ce dossier sont des fragments réutilisables, pas des
appareils à compiler séparément. Les configurations des appareils restent à la
racine du dépôt pour être gérées par l'interface web.

`base.yaml` fournit les journaux, l'API chiffrée, OTA, le Wi-Fi et un point
d'accès de secours. Il attend la substitution `device_name` et les clés
décrites dans `../secrets.yaml.example`.

Depuis un fichier d'appareil à la racine :

```yaml
packages:
  base: !include packages/base.yaml
```
