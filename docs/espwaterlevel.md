# ESPWaterLevel

Configuration : `espwaterlevel.yaml`, ESP32-S2 (`esp32-s2-saola-1`, ESP-IDF).

## Connexions et calibration

| Fonction | Broche |
| --- | --- |
| Déclenchement ultrason | GPIO18 |
| Écho ultrason | GPIO16 |
| LED d'alerte | GPIO15 |

La distance capteur–eau est mesurée toutes les **5 secondes**, avec une
médiane glissante de **9 échantillons**. Ce lissage conserve une inertie
d'environ 20 secondes lors d'un changement brusque, une fois la fenêtre remplie.

La substitution `hauteur_vide_mm` conserve la calibration de **63 mm** :

```text
niveau_mm = borner(63 - distance_mm, 0, 63)
niveau_percent = niveau_mm × 100 / 63
```

Le timeout ultrason `2m` représente une **distance maximale de 2 mètres**,
pas une durée de deux minutes. Ces paramètres matériels sont conservés.

## Traitement des mesures

Le capteur ultrason est l'unique source périodique. Trois capteurs `copy`
calculent la distance en mm, le niveau en mm et le pourcentage à partir
du même échantillon filtré, sans trois interrogations périodiques indépendantes.

La distance en mm est désormais publiée toutes les 5 secondes, comme les
deux niveaux. Auparavant, elle n'était actualisée que toutes les 300 secondes
et les niveaux réutilisaient cette valeur périmée.

La validité du dernier écho est mémorisée **avant** la médiane : celle-ci
ignore les NaN et pourrait sinon ressortir une ancienne valeur après un
timeout. En cas d'échec courant, les trois mesures deviennent `NaN` et
la LED passe immédiatement en alerte rapide au traitement de cet écho.
Le prochain écho valide rétablit le fonctionnement normal, avec le lissage
existant. Les mesures absentes ne sont pas converties en zéro.

## LED

| État | Comportement |
| --- | --- |
| Démarrage / mesure invalide | 200 ms allumée, 200 ms éteinte |
| Niveau < 5 mm | 200 ms allumée, 200 ms éteinte |
| 5 mm ≤ niveau < 10 mm | 800 ms allumée, 800 ms éteinte |
| Niveau ≥ 10 mm | Éteinte |

Les effets natifs `strobe` remplacent les deux boucles de scripts. Un changement
d'effet n'est envoyé que si le mode de la LED change : le clignotement n'est
plus arrêté puis relancé toutes les 5 secondes. Les seuils sont réglables via
`seuil_critique_mm` et `seuil_bas_mm` ; leurs valeurs sont vérifiées à la compilation.

## MQTT et API

Broker : `192.168.50.227`. Contrat : [standard MQTT v1](MQTT_STANDARD.md),
implémenté par `packages/mqtt.yaml`. Les noms des entités sont conservés.
Topics canoniques :

- `device/espwaterlevel_01/sensor/distance_eau_capteur/state`
- `device/espwaterlevel_01/sensor/niveau/state`
- `device/espwaterlevel_01/sensor/niveau_percent/state`
- `device/espwaterlevel_01/sensor/esp_ip_address/state`

Le pourcentage est également publié sous l'ancien nom `niveau_pourcentage`
comme alias retained, y compris `None` en cas de mesure invalide. Le topic
canonique `niveau_percent` correspond désormais au nom annoncé dans le registre
et à l'abonnement Node-RED existant. Les autres topics ne changent pas.

L'annonce JSON versionnée sur `system/announce` est émise à chaque connexion
et toutes les 60 secondes. Une copie retained est publiée sur
`device/espwaterlevel_01/announce`. Les quatre capteurs sont déclarés, avec
des unités et topics explicites ; `inputs` reste vide.
Le résumé JSON `device/espwaterlevel_01/state` est publié toutes les 5 secondes,
avec des nombres ou `null`, et un booléen `mesure_valide`.

La disponibilité est `online` / `offline` (birth, last will, shutdown), retained.
Le heartbeat est `alive`, non retained, toutes les 60 secondes. La découverte
Home Assistant précédemment active est conservée via `mqtt_discovery: "true"`.

L'API est conservée pour l'interface et les journaux, avec `reboot_timeout: 0s` :
un appareil utilisé principalement en MQTT ne doit pas redémarrer au bout de
15 minutes simplement parce qu'aucun client API n'est connecté. Le timeout
de reconnexion Wi-Fi est conservé. Les journaux passent de DEBUG à INFO.

Les identifiants MQTT et le mot de passe du point d'accès sont référencés
depuis `secrets.yaml`, avec les mêmes valeurs qu'auparavant.

## Validation et installation

```bash
esphome config ~/esphome/espwaterlevel.yaml
esphome compile ~/esphome/espwaterlevel.yaml
```

La compilation produit le firmware mais ne l'installe pas. Pour appliquer
la configuration à la carte, utiliser ensuite **Install** dans Device Builder
ou lancer volontairement :

```bash
esphome run ~/esphome/espwaterlevel.yaml --device 192.168.50.32
```

L'adresse `192.168.50.32` est celle observée sur le réseau lors de cette
optimisation ; l'adapter si elle change. Après installation, vérifier le niveau
affiché, les seuils de LED et la reconnexion MQTT avec la carte réelle.
