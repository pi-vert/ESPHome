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

## MQTT

`mqtt.yaml` implémente le [standard MQTT v1](../docs/MQTT_STANDARD.md) :
connexion, disponibilité retained, reconnexion, annonces périodiques,
heartbeat et publication du résumé. Il inclut `includes/mqtt_standard.h`.

Le fichier appareil fournit les substitutions `device_id`, `device_type`,
`firmware_version` et les scripts `mqtt_announce` / `mqtt_snapshot`.
Les capteurs définissent leurs topics explicitement. `espgesture.yaml`,
`espwaterlevel.yaml` et `espwebcam.yaml` sont les implémentations de référence.
