# ESPWebCam

ESP32-CAM dédié à la caméra, avec ESP-IDF, 4 Mo de flash et PSRAM quad à 80 MHz.
Identifiant MQTT : `espwebcam_01` ; version du firmware : `1.1.0`.
Contrat : [standard MQTT v1](MQTT_STANDARD.md).

## Organisation et optimisations

- Le package distant de proxy Bluetooth est retiré, conformément au choix
  « caméra uniquement ». Le scan BLE, le proxy et les boutons hérités de ce
  package ne sont plus chargés. La configuration ne dépend plus de sa branche
  GitHub `main` ni de ses modifications externes.
- `packages/mqtt.yaml` fournit la connexion, la disponibilité, les annonces,
  le heartbeat et le résumé, comme pour ESPGesture et ESPWaterLevel.
- L'annonce de démarrage, qui pouvait partir avant la connexion au broker,
  est remplacée par les annonces standard après connexion et toutes les 60 s.
- L'ancien `i2c_pins` est remplacé par un bus `i2c` dédié, à 100 kHz, sans scan.
- ESPHome dimensionne les sockets selon les composants réellement activés.
  Les anciennes surcharges manuelles HTTP/TCP sont retirées, notamment les
  délais TCP forcés à 1–2 secondes.
- Un seul framebuffer JPEG en PSRAM limite l'occupation de la RAM interne.
- Les éclairages démarrent éteints grâce à `restore_mode: ALWAYS_OFF`, sans
  automation de démarrage redondante. Le flash répond immédiatement, sans
  transition par défaut ; une transition peut encore être demandée explicitement.
- Les réglages d'image API et MQTT passent par la même validation.
- Le point d'accès de secours est désormais explicite, protégé par un secret
  local et accompagné d'un portail captif.

## Profil d'image conservé

| Réglage | Valeur |
| --- | --- |
| Résolution | 640 × 480 |
| Format | JPEG |
| Qualité JPEG | 12 |
| Cadence maximale demandée | 0,5 image/s, soit une image toutes les 2 s |
| Rafraîchissement de veille | 0,2 image/s, soit toutes les 5 s |
| Horloge caméra | 10 MHz |
| Framebuffers | 1, en PSRAM |
| Miroir / retournement | Désactivés |
| Exposition / gain / balance des blancs | Automatiques |

Les substitutions en tête de `espwebcam.yaml` permettent d'ajuster résolution,
qualité et cadence. La cadence réelle dépend aussi du Wi-Fi et de la caméra.
Le rafraîchissement de veille est conservé : le pilote peut garder une ancienne
image en attente, et le premier instantané après une pause n'est pas garanti
nouvellement acquis si ce rafraîchissement est entièrement coupé.

## Broches

- Caméra : XCLK GPIO0 ; SDA GPIO26 ; SCL GPIO27.
- Données D0–D7 : GPIO5, 18, 19, 21, 36, 39, 34, 35.
- VSYNC GPIO25 ; HREF GPIO23 ; PCLK GPIO22 ; PWDN GPIO32.
- Flash blanc : GPIO4, sortie PWM LEDC **canal 2**. Ne pas prendre les canaux
  partageant le timer de l'horloge de la caméra.
- LED de la carte : GPIO33, active à l'état bas.

## Accès aux images

- Flux MJPEG : `http://ADRESSE_IP_CAMERA:8080/`.
- Instantané JPEG : `http://ADRESSE_IP_CAMERA:8081/`.
- Caméra accessible également par l'API ESPHome, avec son nom existant
  `espwebcam_01_camera`.

Les ports sont ceux de l'ancienne configuration. Les images restent sur HTTP/API,
elles ne sont pas transférées dans MQTT. L'annonce contient une liste `services`
avec les URL calculées à partir de l'adresse IP courante. Dans cette version
d'ESPHome, chaque serveur caméra limite lui-même ses connexions ouvertes ; une
surcharge `CONFIG_HTTPD_MAX_OPEN_SOCKETS` ne modifie pas cette limite applicative.

## MQTT

Préfixe : `device/espwebcam_01`.

| Sortie | Topic après le préfixe | Type |
| --- | --- | --- |
| RSSI Wi-Fi | `sensor/wifi/state` | Entier, dBm |
| Statut ESPHome | `binary_sensor/status/state` | ON/OFF |
| Caméra initialisée | `binary_sensor/camera_ready/state` | ON/OFF |
| Flash | `actuator/light/state` | JSON lumière ESPHome |
| LED de la carte | `actuator/led/state` | JSON lumière ESPHome |
| Paramètres d'image | `actuator/camera_config/state` | JSON |
| Switch de redémarrage | `actuator/restart/state` | ON/OFF, non retained |

Les autres états sont retained. `camera_ready` indique que le pilote est
initialisé et non marqué en échec ; il ne garantit pas la fraîcheur du flux.
Le résumé `/state`, toutes les 5 secondes, reprend `wifi`, `camera_ready`,
`light`, `led` et `camera_config`, avec les métadonnées communes du standard.
Un RSSI indisponible vaut `None` sur son topic et `null` dans le résumé.
Les paramètres caméra sont `null` si le pilote est indisponible.

### Commandes, QoS 0 et sans retain

Allumer le flash à environ 50 % :

```text
Topic : device/espwebcam_01/actuator/light/set
Payload : {"state":"ON","brightness":128}
```

Éteindre la LED de la carte :

```text
Topic : device/espwebcam_01/actuator/led/set
Payload : {"state":"OFF"}
```

Modifier le contraste de l'image :

```text
Topic : device/espwebcam_01/actuator/camera_config/set
Payload : {"name":"contrast","value":1}
```

Pour `camera_config`, l'objet doit contenir exactement `name` et `value`.
Noms autorisés : `contrast`, `brightness`, `saturation` ; valeur **entière**
comprise entre **−2 et +2**. Une commande invalide est ignorée avec un journal
WARN, sans modification des paramètres. Une commande valide publie ensuite
l'état connu du pilote et le résumé.

`brightness` dans `camera_config` ajuste l'image ; `brightness` dans la commande
du flash règle l'éclairage, de **0 à 255**. Ce sont deux paramètres distincts.

Redémarrage : `ON` sur `device/espwebcam_01/actuator/restart/set`.

## Compatibilité

L'action API `camera_set_param(name, value)` et les noms des entités caméra,
flash, LED, Wi-Fi, statut et redémarrage sont conservés. Les réglages d'image
ne sont pas persistés en flash : ils reviennent aux valeurs YAML au démarrage.

Les anciens topics automatiques sous `espwebcam/...` sont remplacés par les
topics explicites ci-dessus. Adapter les clients MQTT qui les utilisaient.
Les topics explicites `/status` et `/heartbeat` gardent leur adresse.
La disponibilité est maintenant cohérente pour connexion, arrêt et coupure
brutale, avec une valeur retained.

Le mot de passe de secours est dans `secrets.yaml`, clé
`espwebcam_fallback_ap_password`. L'API reste configurée comme auparavant ;
la clé API historique conservée dans les secrets n'est pas activée implicitement.

## Validation et installation

```bash
g++ -std=c++17 -Wall -Wextra -Werror ~/esphome/tests/camera_controls_test.cpp -o /tmp/esphome-camera-test
/tmp/esphome-camera-test
esphome config ~/esphome/espwebcam.yaml
esphome compile ~/esphome/espwebcam.yaml
```

Après installation volontaire du firmware, vérifier un instantané, le flux,
les deux éclairages et les commandes de paramètres. Les avertissements GPIO0
et GPIO5 concernent les broches de strapping présentes dans le câblage standard
de cette ESP32-CAM.
