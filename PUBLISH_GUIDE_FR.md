# Guide de publication GitHub (pour toi)

Le contenu de ce dossier est prêt à être **déposé dans un dépôt GitHub**. Ne confonds pas le ZIP du code avec le `.exe` final : le vrai `.exe` est créé **par GitHub Actions** après l'envoi du projet.

## 1. Mettre à jour ton dépôt existant

1. Ouvre ton dépôt GitHub existant : https://github.com/Arbusys/PokeLDN-PocketCalm
2. Garde la v0.6 comme sauvegarde (tag ou Release).
3. Remplace/ajoute les fichiers du dossier v0.7 extrait en conservant la structure (y compris `.github/workflows/`).
4. Fais un commit sur `main`. GitHub Actions démarre normalement tout seul. Tu peux aussi utiliser GitHub Desktop : https://desktop.github.com/.

## 2. Lancer la fabrication automatique de l'EXE

1. Ouvre l'onglet **Actions** du dépôt.
2. Clique sur **Build portable Windows firmware installer**.
3. Clique **Run workflow** (si le premier commit n'a pas déjà lancé le build).
4. Attends la fin du job `build` (statut vert).
5. Télécharge l'**Artifact** nommé `PocketCalm-v0.7-Windows-Installer`. Il contiendra `PokeLDN-PocketCalm-v0.7-Installer.exe`.

La compilation est faite sur les serveurs GitHub : les utilisateurs finaux n'ont besoin ni d'ESP-IDF ni de Python.

## 3. Créer une Release pour le public

Après le premier build réussi, pousse un tag `v0.7.0` : cela crée automatiquement une Release et ajoute l'EXE. La publication du tag peut se faire via GitHub Desktop / terminal Git :

```powershell
git tag v0.7.0
git push origin v0.7.0
```

Attends que l'Action finisse avant de partager le lien Releases.

## 4. Ce que fait l'installeur

- Fenêtre Windows avec port COM (détection ou saisie).
- Vérifie la communication avec l'ESP32-S3 avant de flasher.
- Affiche un avertissement et demande confirmation.
- Charge le binaire complet du firmware v0.7 grâce à `esptool.exe`, fourni **dans** le programme.
- Informe l'utilisateur de la réussite ou de l'échec.

**Important :** le flash écrase le firmware de l'appareil. La vérification ESP32-S3 ne permet pas de reconnaître de manière certaine le modèle LILYGO. Le programme est réservé au **LILYGO T-Dongle S3 original à écran intégré 160×80**.

## 5. Ce qui reste à valider

Le pipeline de la v0.6 a été testé avec succès sur ton T-Dongle S3. **La v0.7 reste à compiler sur GitHub puis à tester sur des salons de raid réels** avant publication. Si GitHub Actions échoue, partage les logs.
