# Standard MQTT des appareils — version 1

Référence pour tous les futurs développements de ce dépôt.
Version du contrat : **1**, déclarée par `schema_version: 1` dans les JSON.
Les versions de firmware sont indépendantes (`firmware_version`).

Implémentation commune : `packages/mqtt.yaml` et `includes/mqtt_standard.h`.
Appareils de référence : `espgesture`, `espwaterlevel_01` et `espwebcam_01`.

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
| Demande de découverte | `system/discover` | `announce` | 1 | **false** |
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
immédiatement les descriptifs mémorisés. Le package écoute aussi `system/discover`
(payload `announce`) et répond avec une annonce et un résumé.

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
    "min": 0,
    "max": 100,
    "step": 1,
    "ttl_s": 180,
    "kind": "state",
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

### ESPWebCam

- Capteur : `wifi` (int, dBm).
- États binaires : `status`, `camera_ready`. Ce dernier indique l'initialisation
  du pilote, pas une mesure de fraîcheur des images.
- Actionneurs : `light`, `led`, `camera_config` (JSON), `restart` (commande ON).
- Résumé : `wifi`, `camera_ready`, `light`, `led`, `camera_config`.
- Lumières : JSON natif ESPHome, par exemple `{"state":"ON","brightness":128}`
  pour le flash ; échelle de luminosité 0–255.
- Réglage caméra : `{"name":"contrast","value":1}` ; exactement les champs
  `name` et `value`, avec `contrast`, `brightness` ou `saturation` et un entier
  entre −2 et +2. Les trois paramètres courants sont publiés sur l'état JSON.

Une annonce peut ajouter une liste **`services`** pour les fonctions accessibles
par un autre protocole. Chaque entrée contient `name`, `protocol`, `url` et
`content_type`. Pour ESPWebCam : `stream` en MJPEG sur HTTP 8080 et `snapshot`
en JPEG sur HTTP 8081. Ces services ne sont pas des sorties MQTT routables et
les images ne sont pas insérées dans les messages MQTT.

Les noms d'entités et l'action API de réglage de la caméra sont conservés.
Les anciens topics automatiques `espwebcam/...` sont remplacés par les topics
explicites `device/espwebcam_01/...`. Voir [espwebcam.md](espwebcam.md) pour
les commandes et les paramètres matériels.

Les aliases ne doivent pas servir de modèle aux nouveaux appareils. Leur retrait
nécessite une migration explicite des consommateurs. Aucun effacement automatique
des anciens retained du broker n'est effectué lors d'une mise à jour de firmware.

### ESPMatrix

`espmatrix` expose l'entrée `frame` (JSON) sur
`device/espmatrix/actuator/frame/set`. Le payload est un objet contenant `pixels`,
une liste de **192 entiers de 0 à 255** : R, G, B pour les 64 pixels, dans l'ordre
logique de la matrice. La rotation matérielle reste appliquée par le firmware.
Le schéma est publié dans le descriptif. L'alias historique CSV `matrix/frame`
reste accepté mais n'est pas annoncé comme un second port.

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

## 8. Métadonnées de ports et interconnexion Patchwork

Cette extension est **compatible avec `schema_version: 1`** : les topics et les
payloads existants ne changent pas. Les nouveaux appareils décrivent leurs ports
assez précisément pour créer des liaisons sans connaître leur firmware.

### Descriptif

À la racine, `label` est le nom convivial et `heartbeat_interval_s` l'intervalle
de présence (60 s par défaut). `id` reste l'identité stable du nœud. Tous les
identifiants/noms de port ont au plus 64 caractères, en minuscules ASCII, chiffres,
`_` ou `-`, avec une lettre ou un chiffre en premier ; préférer `_` dans ESPHome.

Chaque port fournit `name`, `datatype`, `category`, `topic`, `qos`, `retain` et,
selon ses besoins :

| Champ | Sens |
| --- | --- |
| `label`, `description` | Nom convivial et signification de la donnée. |
| `unit` | Unité de la valeur transmise, après filtres ESPHome. |
| `min`, `max` | Bornes numériques croissantes ; à fournir dès qu'elles sont connues, notamment pour une consigne. |
| `step` | Résolution/pas positif dans cette unité. |
| `values` | Liste finie de valeurs autorisées (énumération). |
| `kind` | `state` ou `event`. Un événement n'est jamais retained. |
| `ttl_s` | Durée de validité d'une mesure depuis réception (180 s par défaut). |
| `multiple` | Pour une entrée : `single` (défaut), `latest` ou `keyed`. |
| `schema` | Pour du JSON : forme et contraintes des champs. |

Une donnée numérique n'inclut jamais son unité dans le payload. Si les bornes
réelles sont inconnues, les omettre : Patchwork demandera des bornes explicites
pour une conversion de plage. `min/max` n'est pas le minimum/maximum observé depuis
le démarrage ; c'est la plage du signal/consigne.

Le helper `output` retourne maintenant le `JsonObject` du port, comme `input`.
`range(port, min, max, step)` renseigne les bornes. Exemple potentiomètre :

```cpp
auto port = mqtt_standard::output(root, "${mqtt_prefix}", "sensor",
                                 "position", "float", "%", true);
mqtt_standard::range(port, 0, 100, 0.1);
port["label"] = "Position";
port["ttl_s"] = 15;
```

Exemple servo :

```cpp
auto port = mqtt_standard::input(root, "${mqtt_prefix}", "angle", "float");
mqtt_standard::range(port, 0, 180, 1);
port["unit"] = "°";
port["multiple"] = "single";
```

Les modèles complets sont `patchwork-potentiometer.yaml.example` et
`patchwork-servo.yaml.example`, à copier à la racine en `.yaml` et adapter à la
carte/câblage. Le servo reçoit un nombre scalaire en degrés et vérifie également
ses bornes dans le firmware. Sa sortie `angle` est la **consigne appliquée**, pas
une mesure de position physique.

### Cardinalité et conversion

- Une sortie alimente plusieurs entrées : une conversion indépendante par liaison.
- `single` : un seul émetteur actif pour cette entrée (servo, relais).
- `latest` : plusieurs émetteurs ; la dernière valeur reçue est appliquée, sans
  priorité implicite. À déclarer seulement si ce comportement est voulu.
- `keyed` : entrée de type **json**, objet regroupé par clé de liaison. Adapté à
  un écran recevant plusieurs sondes. Chaque entrée contient `value`, `unit`,
  `source`, `port` et `received_at` (date UTC de réception par le routeur).

```json
{
  "salon": {"value": 21.5, "unit": "°C", "source": "sonde_salon", "port": "temperature", "received_at": "2026-09-20T12:00:00.000Z"},
  "jardin": {"value": 17, "unit": "°C", "source": "sonde_jardin", "port": "temperature", "received_at": "2026-09-20T12:00:01.000Z"}
}
```

L'absence d'une clé signifie que sa source est invalide, hors ligne ou périmée.
Le récepteur doit remplacer le regroupement précédent, pas fusionner les clés
absentes indéfiniment. `{}` signifie qu'aucune mesure fraîche n'est disponible.

Patchwork propose transmission directe, interpolation de plage (inversible),
modèle texte, seuil vers booléen et extraction d'un champ JSON. L'entrée impose
son pas et ses bornes. Le mode texte n'exécute aucun code. Les liaisons cycliques
entre appareils sont refusées. Les liens en erreur restent visibles et peuvent
être corrigés ; une nouvelle annonce remplace le descriptif de ports du nœud.

### Découverte et fraîcheur

Une annonce retained permet de reconstruire la carte, mais ne prouve pas que
l'appareil est en ligne. Le routeur attend une annonce fraîche, un heartbeat,
un statut `online` frais ou une mesure fraîche. Une absence de heartbeat durant
trois intervalles rend le nœud non confirmé. `offline` le rend indisponible
immédiatement. Aucune mesure retained ne déclenche d'action, même un événement.
Les états mémorisés sont affichés séparément et aucun ordre n'est rejoué au
redémarrage du routeur ou lors de la création d'une liaison.

Avec MQTT 5, les consommateurs utilisent **Retain As Published = false** : une
mesure publiée en retain par un capteur reste une nouvelle mesure pour un client
déjà abonné. Le flag retained doit signaler uniquement une livraison depuis le
stockage du broker. Les commandes `/set` restent toujours non retained.

### Nœuds virtuels et adaptateurs

Le contrat ne dépend ni d'ESP32 ni d'ESPHome. Un service Internet, un Raspberry Pi
ou un adaptateur AWTRIX utilise la même annonce et la même disponibilité. Un
adaptateur traduit les topics dédiés vers le protocole natif du composant.
Les images HTTP et autres services non scalaires restent dans `services`.

Patchwork fournit deux adaptateurs gérés dans Node-RED : AWTRIX (texte et écran
multi-sources) et météo Open-Meteo (température, humidité, vent, code météo).
Les services MQTT externes peuvent s'annoncer directement ou disposer d'un
descriptif configuré dans l'interface ; ils publient eux-mêmes leur présence.
Ne pas faire démarrer deux routeurs actifs avec les mêmes liaisons sur le même
broker : chacun publierait ses commandes.

Interface : `http://192.168.50.75:1880/patchwork/`. Le détail de l'exploitation,
de la persistance et des conversions est dans le README de l'application
`~/.node-red-clean/projects/ESP32/uibuilder/patchwork/README.md`.
