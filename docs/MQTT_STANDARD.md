# Standard MQTT des appareils — version 1

Référence pour tous les futurs développements de ce dépôt.
Version du contrat : **1**, déclarée par `schema_version: 1` dans les JSON.
Les versions de firmware sont indépendantes (`firmware_version`).

Implémentation commune : `packages/mqtt.yaml` et `includes/mqtt_standard.h`.
Appareils de référence : `espgesture` et `espwaterlevel_01`.

## 1. Identifiants et topics

- Identifiants techniques stables en minuscules ASCII : lettres, chiffres et `_`.
- `device_id` identifie l'appareil physique ; il est unique sur le broker.
- Le libellé affiché (`name` d'une entité ESPHome) peut changer sans changer son topic.
- Un nom de sortie est unique au sein d'un appareil, toutes catégories confondues.
- Chaque entité publique définit explicitement son `state_topic` et, le cas
  échéant, son `command_topic`. Ne pas dériver le contrat MQTT des noms affichés.

| Usage | Topic | Payload | QoS | Retain |
| --- | --- | --- | --- | --- |
| Annonce commune | `system/announce` | JSON d'un appareil | 1 | **false** |
| Descriptif persistant | `device/<id>/announce` | Même JSON | 1 | true |
| Disponibilité | `device/<id>/status` | `online` / `offline` | 1 | true |
| Battement de vie | `device/<id>/heartbeat` | `alive` | 0 | false |
| Capteur | `device/<id>/sensor/<sortie>/state` | Nombre ou texte déclaré | 0 | true |
| État binaire | `device/<id>/binary_sensor/<sortie>/state` | `ON` / `OFF` | 0 | Selon sa nature |
| Événement | `device/<id>/event/<sortie>/state` | Valeur déclarée | 0 | **false** |
| État d'un actionneur | `device/<id>/actuator/<entrée>/state` | Valeur déclarée | 0 | true |
| Commande | `device/<id>/actuator/<entrée>/set` | Valeur déclarée | 0 recommandé | **false** |
| Résumé | `device/<id>/state` | JSON | 0 | true |
| Diagnostic | `device/<id>/debug/state` ou `/debug/gesture_scores` | JSON | 0 | false |

Le suffixe `/state` des événements est volontaire : le routeur Patchwork/Node-RED
existant sait router `device/<id>/<catégorie>/<sortie>/state`.
**Le champ `name` de la sortie annoncée doit être exactement `<sortie>` dans
le topic.** Ne pas annoncer `niveau_percent` en publiant uniquement sous
`niveau_pourcentage`.

Les états binaires représentant des impulsions de geste utilisent `retain: false`.
L'état transitoire du switch de redémarrage utilise également `retain: false`.
Les unités sont des métadonnées, jamais ajoutées au nombre transmis.

## 2. Annonce et découverte

Publier l'annonce **après la connexion MQTT**, pas à l'événement de connexion
Wi-Fi : le broker n'est pas forcément connecté à cet instant.
Répéter l'annonce à chaque reconnexion et toutes les 60 secondes. Cela permet
au registre existant, abonné uniquement à `system/announce`, de se reconstituer.
Un nouveau consommateur peut s'abonner à `device/+/announce` pour obtenir
immédiatement les descriptifs mémorisés.

`system/announce` n'est jamais retained : un topic commun retained ne garderait
que le dernier appareil. La copie persistante appartient au topic de l'appareil.

Format **plat**, un appareil par message ; ne pas envelopper sous une clé
portant le nom de l'appareil :

```json
{
  "schema_version": 1,
  "id": "espwaterlevel_01",
  "type": "water_level",
  "firmware_version": "1.2.0",
  "uptime_s": 123,
  "ip": "192.168.50.32",
  "availability_topic": "device/espwaterlevel_01/status",
  "state_topic": "device/espwaterlevel_01/state",
  "outputs": [
    {
      "name": "niveau_percent",
      "category": "sensor",
      "datatype": "float",
      "unit": "%",
      "topic": "device/espwaterlevel_01/sensor/niveau_percent/state",
      "qos": 0,
      "retain": true
    }
  ],
  "inputs": []
}
```

Cet exemple ne montre qu'une sortie ; l'annonce réelle expose toutes les
sorties contractuelles. `outputs` et `inputs` sont toujours des tableaux,
y compris lorsqu'ils sont vides. Les aliases de migration ne sont pas annoncés,
afin de ne pas créer de sorties en double dans le registre.

Pour une entrée, fournir `name`, `datatype`, `topic`, `qos`, `retain` et
`values` lorsque les valeurs autorisées sont une liste finie. Exemple :

```json
{
  "name": "debug_mode",
  "datatype": "string",
  "topic": "device/espgesture/actuator/debug_mode/set",
  "qos": 0,
  "retain": false,
  "values": ["ON", "OFF"]
}
```

## 3. Types et valeurs indisponibles

- `float` : nombre décimal, point comme séparateur, aucune unité dans le payload.
- `int` : entier décimal. Exemple : RSSI Wi-Fi `-62`, unité annoncée `dBm`.
- `string` : texte UTF-8 ; les gestes sont `UP`, `DOWN`, `LEFT`, `RIGHT`.
- `bool` : représenté par `ON` / `OFF` sur les topics d'état binaire,
  par un booléen JSON dans les résumés.
- `json` : objet JSON valide, jamais une chaîne contenant du JSON échappé.

Pour les capteurs numériques natifs ESPHome, une mesure invalide est le texte
**`None`**, grâce à `publish_nan_as_none: true`. Dans les JSON, elle est
**`null`**. Ne jamais publier `NaN`, `Infinity`, ni remplacer une panne par zéro.
Un consommateur doit traiter ces valeurs comme indisponibles avant tout calcul
ou routage vers un actionneur.

Les champs de couleur, lumière claire et proximité de l'APDS9960 sont des
**pourcentages**, pas des valeurs brutes ni des lux. Leur résolution MQTT est
fixée à une décimale. Les niveaux d'eau restent affichés sans décimales.

## 4. États et événements : ne pas les confondre

Un état retained est une **dernière valeur connue**, pas une garantie de fraîcheur.
Le consommateur doit aussi consulter la disponibilité et l'âge de réception.
Le JSON de résumé conserve les mesures à la racine pour la compatibilité :

```json
{
  "schema_version": 1,
  "id": "espwaterlevel_01",
  "type": "water_level",
  "firmware_version": "1.2.0",
  "uptime_s": 123,
  "distance_eau_capteur": 40.0,
  "niveau": 23.0,
  "niveau_percent": 36.5079,
  "mesure_valide": true
}
```

`uptime_s` est un compteur monotone 64 bits de secondes depuis le démarrage,
**pas** une date UTC ni l'heure exacte d'acquisition de toutes les mesures.
Les résumés sont publiés toutes les 5 secondes par défaut et à la connexion.
Ils peuvent conserver une précision supérieure à l'affichage des topics scalaires.
Les consommateurs ignorent les champs supplémentaires qu'ils ne connaissent pas.

Un geste, lui, est un **événement** :

- publication non retained, QoS 0 ; pas de garantie de livraison en cas de coupure ;
- chaque geste validé est émis, y compris deux `UP` consécutifs ;
- pas de rejeu volontaire à la reconnexion ; ne pas utiliser un capteur texte
  MQTT natif sur un topic d'événement, car il republie son dernier état à la connexion ;
- `sensor/last_gesture/state` mémorise séparément le dernier geste pour l'affichage.
  Ce topic et le champ `relevant_gesture` du résumé ne doivent pas déclencher une action.

Les diagnostics sont publiés uniquement lorsque `debug_mode` est activé,
sans retain. Le mode diagnostic utilise le même état que l'interrupteur ESPHome,
afin de rester cohérent après restauration du firmware.

## 5. Disponibilité et commandes

Le package commun configure `birth_message=online`, `will_message=offline`
et `shutdown_message=offline`, tous sur le même topic de statut, QoS 1 retained.
`keepalive` vaut 30 secondes ; la détection d'une coupure brutale dépend du broker.
Le heartbeat `alive`, non retained, est émis toutes les 60 secondes.

Les commandes ne sont jamais retained. Les commandes `ON` / `OFF` du mode
diagnostic sont idempotentes ; ne pas utiliser `TOGGLE` dans ce contrat.
Le redémarrage accepte `ON`, QoS 0 : éviter toute répétition automatique.
Le package utilise une session MQTT propre, pour ne pas accumuler des commandes
destinées à un appareil déconnecté. Une session propre ne supprime pas les
commandes retained : leur absence reste une obligation de l'émetteur.

## 6. Sorties et compatibilité des appareils actuels

### ESPGesture

- Capteurs : `clear`, `proximity`, `red`, `green`, `blue` (float, `%`),
  `wifi` (int, `dBm`), `last_gesture` (string).
- États binaires : `gesture_up`, `gesture_down`, `gesture_left`, `gesture_right`
  (impulsions non retained), `status` (état binaire retained).
- Événements : `gesture_raw`, `gesture_relevant` (string, non retained).
- Actionneurs : `debug_mode` (ON/OFF), `restart` (commande ON).
- Résumé : champs `clear`, `proximity`, `red`, `green`, `blue`, `wifi`,
  `relevant_gesture`, `debug_mode`, plus les métadonnées communes.

Les anciens topics explicites `/gesture/raw` et `/gesture/relevant` sont encore
émis comme **aliases non retained**, avec exactement la même valeur que le topic
canonique. Un consommateur choisit l'un ou l'autre, jamais les deux pour une action.
L'alias `/debug/status` garde `enabled` / `disabled` pour les anciens clients.

Les anciennes publications automatiques sous `espgesture/...` sont remplacées
par les topics explicites `device/espgesture/...`. Les clients qui utilisaient
ces anciens topics automatiques doivent adapter leurs abonnements. Les noms des
entités de l'API ESPHome sont conservés.

### ESPWaterLevel

- Capteurs : `distance_eau_capteur` (float, `mm`), `niveau` (float, `mm`),
  `niveau_percent` (float, `%`), `esp_ip_address` (string).
- Résumé : `distance_eau_capteur`, `niveau`, `niveau_percent`, `mesure_valide`.
- Aucune entrée/actionneur annoncée.

Le topic canonique du pourcentage est désormais :
`device/espwaterlevel_01/sensor/niveau_percent/state`, cohérent avec le nom
de la sortie et avec l'abonnement Node-RED existant.
L'ancien `device/espwaterlevel_01/sensor/niveau_pourcentage/state` reste publié
comme **alias retained**, y compris pour une mesure invalide (`None`).
Les topics de distance, niveau et adresse IP restent identiques.

Les aliases ne doivent pas servir de modèle aux nouveaux appareils. Leur retrait
nécessite une migration explicite des consommateurs. Aucun effacement automatique
des anciens retained du broker n'est effectué lors d'une mise à jour de firmware.

## 7. Créer un nouvel appareil conforme

1. Choisir un `device_id` et des noms de sorties stables.
2. Définir `device_type` et `firmware_version` dans `substitutions`.
3. Inclure `packages/mqtt.yaml` et conserver les identifiants dans `secrets.yaml`.
4. Définir les scripts `mqtt_announce` et `mqtt_snapshot`, appelés par le package.
5. Construire les JSON avec `mqtt_standard::announcement`, `output`, `input`,
   `envelope` et `number` dans `includes/mqtt_standard.h`, comme les appareils
   de référence. Le helper `number` convertit les nombres non finis en `null`.
6. Définir explicitement les topics, QoS et retain des capteurs/actionneurs.
7. Déclarer toutes les sorties routables et toutes les commandes dans l'annonce.
8. Valider, compiler, puis vérifier la réception après installation et reconnexion.

```yaml
substitutions:
  device_id: mon_capteur_01
  device_type: temperature
  firmware_version: "1.0.0"

packages:
  mqtt_standard: !include packages/mqtt.yaml
```

Ce fragment est à compléter avec le matériel, le réseau, les sorties et les deux
scripts. `mqtt_discovery` permet d'activer séparément la découverte Home Assistant ;
elle ne remplace pas le contrat ci-dessus. Les logs MQTT natifs sont désactivés
dans le package pour éviter un flux parallèle non normalisé ; les logs série/API
restent disponibles.

Toute modification incompatible de format, unité, sens, ou politique de rejeu
doit être documentée et versionnée. Préférer l'ajout de champs compatibles ou
un alias temporaire annoncé dans les notes de migration.
