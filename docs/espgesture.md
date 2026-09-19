# ESPGesture

ESP32-C3 Super Mini, flash 4 Mo, ESP-IDF et APDS9960 sur I²C :
SDA GPIO4, SCL GPIO3, adresse `0x39`. LED d'état sur GPIO8.
Les réglages optiques et la cadence de mesure d'une seconde sont conservés.

Le [standard MQTT v1](MQTT_STANDARD.md) est partagé avec ESPWaterLevel.

## Corrections

- Annonce au moment où **MQTT** est connecté, avec l'IP courante, puis toutes
  les 60 secondes. L'ancien déclenchement Wi-Fi pouvait publier avant la
  connexion au broker. L'objet annoncé est désormais plat et versionné.
- Les capteurs numériques et binaires publient directement sur leurs topics
  explicites avec le composant MQTT natif. Les doubles publications
  automatiques/manuelles et les conversions `str_sprintf` sont supprimées.
- Couleurs, lumière claire et proximité : pourcentages à une décimale,
  au lieu d'être arrondis en entiers dans les publications manuelles.
- Résumé toutes les 5 secondes ; diagnostics toutes les 2 secondes uniquement
  lorsque le mode debug est activé. Les nombres non finis deviennent `null`
  dans les JSON, `None` dans les états numériques natifs.
- Un seul état fait foi pour le mode debug : l'interrupteur ESPHome restauré.
  L'ancien booléen global indépendant pouvait contredire sa position.
- Logs INFO ; aucun redémarrage provoqué uniquement par l'absence d'un client
  API ou du broker. Le point d'accès de secours possède désormais un portail captif.

## Sélection d'un geste

Les quatre directions appellent le même script `record_gesture`.
Le premier événement ouvre une fenêtre de **250 ms** (`gesture_window`).
Les suivants alimentent les compteurs sans repousser indéfiniment sa fermeture.
À la fin de la fenêtre, un seul geste est choisi, puis les compteurs sont remis
à zéro pour la fenêtre suivante.

La direction qui possède le score maximal gagne. En cas d'égalité, c'est la
première direction arrivée **parmi celles au score maximal** qui gagne.
Exemple : `UP, DOWN, LEFT, DOWN, LEFT` produit `DOWN`, pas `UP`.
L'ancien code pouvait choisir le premier événement même avec un score inférieur.

La logique est isolée dans `includes/gesture_window.h`, avec un test de
régression pour l'égalité, les directions invalides, la remise à zéro et les
gestes identiques sur deux fenêtres successives :

```bash
g++ -std=c++17 -Wall -Wextra -Werror ~/esphome/tests/gesture_window_test.cpp -o /tmp/esphome-gesture-test
/tmp/esphome-gesture-test
```

## Événements et affichage

Les événements routables sont :

- `device/espgesture/event/gesture_raw/state`
- `device/espgesture/event/gesture_relevant/state`

Ils transportent `UP`, `DOWN`, `LEFT` ou `RIGHT`, QoS 0, **non retained**.
Deux gestes `UP` sur deux fenêtres donnent bien deux événements. Les événements
survenus hors connexion ne sont pas rejoués par un capteur texte MQTT.
Une connexion ou déconnexion MQTT annule également la fenêtre de sélection
en cours, afin de ne pas émettre un geste ancien juste après une reconnexion.

Le dernier geste est conservé séparément pour l'affichage dans l'API ESPHome
et sur `device/espgesture/sensor/last_gesture/state` (retained). Il peut être
republié à la reconnexion, mais ne constitue pas un nouvel événement.

Les topics historiques explicites `device/espgesture/gesture/raw` et
`device/espgesture/gesture/relevant` restent émis comme aliases non retained.
Les anciens topics automatiques sous `espgesture/...` sont remplacés par les
topics explicites du standard ; adapter les clients qui les utilisaient.

## Commandes

- `device/espgesture/actuator/debug_mode/set` : `ON` ou `OFF`, non retained.
- `device/espgesture/actuator/restart/set` : `ON`, non retained.

Le mode debug dispose d'un état retained `actuator/debug_mode/state` (ON/OFF).
L'ancien alias `device/espgesture/debug/status` conserve enabled/disabled.
L'état du switch de redémarrage est transitoire, non retained.

## Validation

```bash
esphome config ~/esphome/espgesture.yaml
esphome compile ~/esphome/espgesture.yaml
```

Après installation volontaire du firmware, vérifier les quatre directions,
les gestes répétés, le mode debug et une reconnexion MQTT. GPIO8 reste une
broche de strapping de cette carte : la compilation ESPHome le signale comme
dans la configuration d'origine.
