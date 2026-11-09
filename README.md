# CK - Configuration Scénario

Plugin Workbench (Arma Reforger Tools) pour préparer un scénario CK. Avec lui, vous :

1. ajoutez au projet les dépendances CK : CK Core, CK Props, CK Structures (et d'autres mods si besoin) ;
2. choisissez le terrain, puis donnez le nom, la date, le lieu et les scénaristes de l'opération ;
3. renommez le mod `CK_Op_<année>_<mois>_<jour>`, mois et jour sur 2 chiffres (le 2 décembre 2026 donne `CK_Op_2026_12_02`), avec un redémarrage de Workbench si besoin ;
4. créez le monde du scénario dans le mod : `Worlds/CK_Op_2026_12_02/CK_Op_2026_12_02.ent` (sous-scène du terrain) ;
5. créez les calques de l'opération : `Composants_Mission`, `Base_Avancee`, `Jour_1` à `Jour_4` ;
6. lancez Game Mode Setup, toujours en Game Master, avec un mission header au nom de l'opération, signé par vos scénaristes, avec la description et les images CK ;
7. placez les modules CK (troops, infos de mission, intro au nom et au lieu de l'opération), les arsenaux et le point de spawn « Base avancée », avec un briefing et des loadouts CK propres au scénario, dont la « Tenue de briefing ». Le monde est enregistré à la fin.

## Prérequis

- Un projet de scénario créé dans Workbench (le nom n'a pas d'importance, le plugin le renomme).
- Les mods CK Core, CK Props et CK Structures installés (Workshop ou dossier ajouté au launcher Workbench).

## Installation

1. Copiez les dossiers `Scripts` et `UI` de ce dossier dans le dossier du projet du scénario, à côté de `addon.gproj`. Si le projet a déjà ces dossiers, ils se fusionnent.
   Résultat attendu :
   ```
   <projet>\Scripts\WorkbenchGame\CK_ConfigurationScenario\CK_ConfigurationScenarioPlugin.c
   <projet>\UI\Textures\CK_Scenario\CK_MissionLoadingScreen.edds (+ .png, .meta)
   <projet>\UI\Textures\CK_Scenario\CK_MissionPreview.edds (+ .png, .meta)
   ```
2. Ouvrez le projet dans Workbench. S'il était déjà ouvert, redémarrez Workbench (les images doivent être prises en compte).
3. Le plugin apparaît dans le menu **Plugins** de la fenêtre principale et du World Editor : **CK - Configuration Scénario**.

Le plugin doit être dans le projet du scénario lui-même : Workbench ne compile que les scripts du projet ouvert et de ses dépendances. Le dossier `UI` contient l'écran de chargement et l'aperçu du scénario : sans lui, le mission header garde les images par défaut (le plugin le signale).

## Utilisation

Lancez **Plugins > CK - Configuration Scénario**. La première fois dans une session, Workbench demande d'autoriser le script (il lit et modifie `addon.gproj`) : acceptez. Si vous avez refusé, redémarrez Workbench.

1. **Dépendances** : les 3 mods CK sont ajoutés au projet. Répondez **Oui** pour ajouter d'autres mods, et collez leurs GUID (un par ligne, ou séparés par des virgules).
2. **Terrain** : choisissez le monde du terrain (`.ent`), par ex. Everon ou Arland. Le scénario sera une sous-scène de ce terrain. Un monde qui a déjà un mode de jeu (par ex. `GM_Eden`, `GM_Arland`) convient aussi : Game Mode Setup le trouve valide et ne crée rien, le plugin utilise ce mode de jeu. Ce mode de jeu appartient au terrain et le scénario ne peut pas le modifier : les factions US et USSR ne sont pas jouables au départ, le Game Master les active en jeu.
3. **Opération** : saisissez le nom de l'opération, choisissez la date, puis indiquez le lieu (ville et/ou pays) et les scénaristes (séparés par des virgules). Lieu vide : « Quelque part dans le monde ». Scénaristes vides : « Commando Kieffer ». Le plugin affiche le récapitulatif et le nom du mod obtenu : cliquez sur **Valider**, ou sur **Modifier** pour corriger.
4. **Redémarrage** : cliquez sur **Redémarrer Workbench** (enregistrez votre travail avant). Workbench se ferme, le dossier du mod est renommé, puis Workbench rouvre le projet renommé et le plugin reprend tout seul. Dans la liste du launcher Workbench, le projet renommé remplace l'ancien chemin (ou y est ajouté). Si Workbench ne se rouvre pas sur le projet, ouvrez-le depuis le launcher, relancez le plugin et cliquez sur **Continuer** : rien n'est perdu.
5. **Monde et calques** : le plugin crée et ouvre `Worlds/<mod>/<mod>.ent`, puis crée les calques de l'opération.
6. **Game Mode Setup** : l'assistant vanilla démarre directement au scan du monde (modèle Game Master imposé). Cliquez sur **Scan world**, **Create entities**, puis **Next** jusqu'au bout. Le mission header est créé (voir plus bas).
7. **Éléments** : ils sont placés devant la caméra du World Editor, chacun dans son calque, et restent sélectionnés pour les déplacer d'un coup. Le monde est enregistré et le calque actif devient `Jour_1`. La dernière fenêtre liste les fichiers à modifier ; **Ouvrir le briefing** l'ouvre dans le Resource Manager.

Ce qui est déjà dans le monde n'est pas placé une deuxième fois : vous pouvez relancer le plugin sans risque.

## Calques du monde

| Calque | Contenu |
|---|---|
| `Composants_Mission` | composants système : mode de jeu Game Master et ses gestionnaires, AI World, Perception Manager, `CK_Troops`, `CK_MissionSettings`, `CK_MissionIntro` |
| `Base_Avancee` | la base avancée (FOB), d'où l'on part : arsenaux, dépôts de véhicules, point de spawn « Base avancée », estrade CK |
| `Jour_1` à `Jour_4` | vides : un calque par jour de l'opération (un vendredi = un jour) |

Le monde n'a pas de calque `default`. Si un monde en a déjà un (créé avec une ancienne version du plugin), le plugin range les entités du scénario dans leurs calques et supprime `default` quand il est vide. Les entités que vous avez mises dans un autre calque ne bougent pas.

## Fichiers du scénario (dans le mod)

| Fichier | À modifier |
|---|---|
| `Worlds/<mod>/<mod>.ent` | le monde du scénario |
| `Missions/<mod>.conf` | mission header : nom (= opération), auteurs (= scénaristes), description, images, nombre de joueurs |
| `Configs/CK/<mod>_Briefing.conf` | briefing du menu CK : titre (= opération), sous-titre avec la date, Situation / Mission / Exécution / Logistique / Transmissions à compléter |
| `Configs/CK/<mod>_Loadouts.conf` | loadouts partagés du menu CK : « Tenue de briefing » en premier, ajoutez les autres à la suite |
| `Prefabs/CK/<mod>_TenueBriefing.et` | le soldat dont l'équipement est la tenue de briefing |
| entité `CK_Troops` du monde | troops et bordées (CK Core n'a pas de `.conf` pour les troops : tout se règle sur l'entité) |
| entité `CK_MissionIntro` du monde | intro au premier spawn : titre (= opération), lieu (= lieu saisi), image, date et heure |

Le briefing et les loadouts sont reliés à `CK_MissionSettings` (composants Briefing et Loadouts), et le briefing aussi au mission header (champ CK Briefing).

## Mission header

| Champ | Valeur |
|---|---|
| Name | nom de l'opération |
| Author | scénaristes saisis (« Commando Kieffer » si vide) |
| Description | « Un scénario du commando Kieffer » |
| Icon | logo CK de CK Core (`UI/Textures/CK/CK_Logo.edds`, comme le `thumbnail.png` des mods CK) |
| Loading Screen | `UI/Textures/CK_Scenario/CK_MissionLoadingScreen.edds` (1920×1080, dossier `UI` du plugin) |
| Preview Image | `UI/Textures/CK_Scenario/CK_MissionPreview.edds` (même image recadrée, 800×800) |

Quand vous relancez le plugin, le nom et les auteurs sont repris de la saisie ; la description et les images ne sont remplacées que si elles sont vides ou encore aux valeurs par défaut de Workbench. Vos propres images et votre description sont donc gardées.

## Tenue de briefing

Premier loadout de la mission (faction US), à charger depuis le menu CK, l'arsenal ou le menu de respawn :

- treillis BME (veste `F3_BME` et pantalon `Pants_VKPO_BME`, mod French Uniform) ;
- rangers de l'armée US (vanilla), pas de casque, pas de gilet ni de sac ;
- Glock 17 de CK Weapons : un chargeur dans l'arme, 2 dans le pantalon ;
- SICS de CK Equipment (à la place de la carte), dans le pantalon ;
- montre vanilla, pas de boussole.

French Uniform, CK Weapons et CK Equipment ne sont pas ajoutés aux dépendances : chargez-les avec le scénario, sinon les objets de ces mods manquent dans la tenue. Pour changer la tenue, modifiez `Prefabs/CK/<mod>_TenueBriefing.et` (composants Loadout Manager, Inventory Storage Manager et emplacement d'arme 2).

## Éléments placés

| Élément | Calque | Mod | Prefab |
|---|---|---|---|
| Système des troops | `Composants_Mission` | CK Core | `Prefabs/CK/CK_Troops.et` |
| Infos de mission du menu CK (briefing, loadouts) | `Composants_Mission` | CK Core | `Prefabs/CK/CK_MissionSettings.et` |
| Intro de la mission | `Composants_Mission` | CK Core | `Prefabs/CK/CK_MissionIntro.et` |
| Estrade avec insigne CK | `Base_Avancee` | CK Props | `Prefabs/Props/Military/Camps/Platform_01_CK.et` |
| Dépôt de véhicules - héliport | `Base_Avancee` | CK Structures | `Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Helipad.et` |
| Dépôt de véhicules - plateforme | `Base_Avancee` | CK Structures | `Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Platform.et` |
| Arsenaux armes, munitions, médical, équipement | `Base_Avancee` | Arma Reforger | `Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_*_US.et` |
| Point de spawn « Base avancée » | `Base_Avancee` | Arma Reforger | `Prefabs/MP/Spawning/SpawnPoint_US.et` |

Factions : en Game Master, aucune faction n'est jouable au départ. Le plugin rend US et USSR jouables dans le gestionnaire de factions du mode de jeu, comme `FactionManager_USxUSSR` (ce prefab n'est pas ajouté : il entrerait en conflit avec le gestionnaire du Game Master). Le spawn « Base avancée » et la tenue de briefing sont pour US : pour jouer USSR, ajoutez-lui un point de spawn et des tenues.

## Si le redémarrage se passe mal

Le redémarrage écrit son déroulé dans `Documents\My Games\ArmaReforgerWorkbench\profile\CK_ConfigurationScenario\restart.log` : dossier avant et après, renommage réussi ou non, dossier du jeu, commande lancée, puis « Workbench rouvert, plugin lancé dans le projet » si le plugin a bien repris. Si vous signalez un problème, joignez ce fichier et le `console.log` de la session rouverte (dernier dossier de `Documents\My Games\ArmaReforgerWorkbench\logs`).

- **Le dossier n'est pas renommé** : un programme l'utilisait (explorateur ouvert dedans, VS Code, OneDrive, antivirus). Le plugin rouvre alors le projet sous son ancien nom et le signale à la fin. Fermez ce programme et renommez le dossier vous-même, Workbench fermé.
- **Un dossier porte déjà le nom du mod** (autre scénario à la même date) : le dossier garde son nom, le plugin le signale avant de redémarrer.
- **Workbench rouvre le launcher** : le plugin n'a pas trouvé le dossier d'Arma Reforger ou le `.gproj`. Ouvrez le projet renommé (en tête de liste), relancez le plugin et cliquez sur **Continuer**.
- **Workbench s'ouvre sur le projet ArmaReforger** : il n'a pas trouvé une dépendance du scénario (son `console.log` indique « dependency … can't be added »). Le plugin donne à Workbench les projets du launcher et les mods téléchargés dans `Documents\My Games\ArmaReforger\addons` : un mod rangé ailleurs n'est pas trouvé. Fermez Workbench, ouvrez le projet depuis le launcher, relancez le plugin et cliquez sur **Continuer**.
- **Workbench ne se rouvre pas, ou s'ouvre sur un autre projet** : fermez-le, ouvrez le projet depuis le launcher (le plugin y a mis le nouveau chemin), puis relancez le plugin et cliquez sur **Continuer**.
- **Ancienne entrée dans le launcher** (projet renommé avec une version précédente du plugin) : retirez-la du launcher ; relancer le plugin sur le projet l'y ajoute sous son nouveau nom.

## Bon à savoir

- Game Mode Setup ne regarde que la sous-scène : sur un terrain qui a déjà une carte (Arland, Everon…), il en crée une deuxième (erreur « Multiple map entities present! » dans la console). Le plugin supprime ce doublon, ainsi que ceux de l'AI World et du Perception Manager si le terrain en a déjà.
- Le menu Plugins contient aussi **CK - Game Mode Setup (Game Master)** : c'est l'assistant vanilla sans le choix du modèle, utilisé par le plugin. Il peut servir seul.
- Avant de modifier `addon.gproj`, le plugin en garde une copie dans `Documents\My Games\ArmaReforgerWorkbench\profile\CK_ConfigurationScenario\` (avec l'état de la configuration en cours).
- Le dossier du plugin peut rester dans le projet publié : le jeu ne charge pas les scripts `WorkbenchGame`.
- Mise à jour : remplacez le fichier `.c` et copiez le dossier `UI`. Sur un scénario déjà configuré, relancez le plugin pour ajouter ce qui manque (intro, tenue de briefing, images du mission header).
- Testé avec Arma Reforger Tools 1.9.
