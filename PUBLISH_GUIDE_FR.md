# Guide de publication GitHub (pour toi)

Le contenu de ce dossier est prêt à être **déposé dans un dépôt GitHub**. Ne confonds pas le ZIP du code avec le `.exe` final : le vrai `.exe` est créé **par GitHub Actions** après l'envoi du projet.

## 1. Créer le dépôt

1. Va sur https://github.com/new et crée un dépôt public, par exemple `pokeldn-pocket-calm`.
2. Laisse le dépôt vide (pas de README automatique : celui-ci en contient déjà un).
3. Ajoute le contenu **extrait** du ZIP au dépôt, en conservant exactement la structure des dossiers (notamment `.github/workflows/`).
   Pour gérer les dossiers sans ligne de commande, tu peux utiliser GitHub Desktop : https://desktop.github.com/
4. Fais un commit sur la branche `main` et pousse-le (Publish/Push).

## 2. Lancer la fabrication automatique de l'EXE

1. Ouvre l'onglet **Actions** du dépôt.
2. Clique sur **Build portable Windows firmware installer**.
3. Clique **Run workflow** (si le premier commit n'a pas déjà lancé le build).
4. Attends la fin du job `build` (statut vert).
5. Télécharge l'**Artifact** nommé `PocketCalm-v0.6-Windows-Installer`. Il contiendra `PokeLDN-PocketCalm-v0.6-Installer.exe`.

La compilation est faite sur les serveurs GitHub : les utilisateurs finaux n'ont besoin ni d'ESP-IDF ni de Python.

## 3. Créer une Release pour le public

Après le premier build réussi, pousse un tag `v0.6.0` : cela crée automatiquement une Release et ajoute l'EXE. La publication du tag peut se faire via GitHub Desktop / terminal Git :

```powershell
git tag v0.6.0
git push origin v0.6.0
```

Attends que l'Action finisse avant de partager le lien Releases.

## 4. Ce que fait l'installeur

- Fenêtre Windows avec port COM (détection ou saisie).
- Vérifie la communication avec l'ESP32-S3 avant de flasher.
- Affiche un avertissement et demande confirmation.
- Charge le binaire complet du firmware v0.6 grâce à `esptool.exe`, fourni **dans** le programme.
- Informe l'utilisateur de la réussite ou de l'échec.

**Important :** le flash écrase le firmware de l'appareil. La vérification ESP32-S3 ne permet pas de reconnaître de manière certaine le modèle LILYGO. Le programme est réservé au **LILYGO T-Dongle S3 original à écran intégré 160×80**.

## 5. Ce qui reste à valider

La compilation du code Windows a été testée, mais le workflow GitHub n'a pas encore été exécuté et l'EXE final n'a pas été essayé sur un PC/ESP32 réel. On ne peut pas présenter cette étape comme déjà validée. Si GitHub Actions affiche une erreur, copie les dernières lignes du log dans ChatGPT.
