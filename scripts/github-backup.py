#!/usr/bin/env python3
"""Sauvegarde horaire des sources ESPHome vers le dépôt GitHub configuré."""

import argparse
from datetime import datetime
import fcntl
import os
from pathlib import Path
import subprocess

import yaml


ROOT = Path(__file__).resolve().parents[1]
REMOTE = "git@github.com:pi-vert/ESPHome.git"
SENSITIVE_FIELDS = {"password", "username", "key", "token", "api_key"}


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args])


def check_yaml(data, label):
    """Refuser les identifiants usuels en clair, sans afficher leur valeur."""
    try:
        documents = list(yaml.compose_all(data))
    except yaml.YAMLError:
        raise SystemExit(f"YAML illisible : {label}. Envoi interrompu.") from None
    seen = set()

    def visit(node):
        if node is None or id(node) in seen:
            return
        seen.add(id(node))
        if isinstance(node, yaml.MappingNode):
            for key, value in node.value:
                if (
                    isinstance(key, yaml.ScalarNode)
                    and key.value.lower() in SENSITIVE_FIELDS
                    and isinstance(value, yaml.ScalarNode)
                    and value.tag != "!secret"
                    and value.tag != "tag:yaml.org,2002:null"
                    and value.value
                ):
                    raise SystemExit(
                        f"Identifiant en clair : {label}, ligne "
                        f"{value.start_mark.line + 1} ({key.value}). "
                        "Utiliser !secret ; si déjà enregistré, nettoyer aussi "
                        "l'historique avant l'envoi."
                    )
                visit(value)
        elif isinstance(node, yaml.SequenceNode):
            for value in node.value:
                visit(value)

    for document in documents:
        visit(document)


def private_path(path):
    name = path.name.lower()
    return (
        "backups" in path.parts
        or any(part in {".esphome", ".venv", ".pio"} for part in path.parts)
        or name.startswith((".device-builder", ".env"))
        or name in {".receiver_peers.json", ".offloader_pairings.json"}
        or path.suffix in {".bin", ".elf", ".uf2", ".hex", ".bundle"}
        or (
            path.suffix in {".yaml", ".yml"}
            and (name.startswith("secrets.") or ".secrets." in name)
        )
    )


def check_history(revision):
    # Tous les objets atteignables : un secret supprimé du dernier commit
    # reste publiable s'il subsiste dans un ancien commit.
    for entry in git("rev-list", "--objects", revision).decode().splitlines():
        oid, separator, name = entry.partition(" ")
        if not separator:
            continue
        path = Path(name)
        if private_path(path):
            raise SystemExit(f"Fichier privé dans l'historique : {name}.")
        if path.suffix in {".yaml", ".yml"}:
            check_yaml(git("cat-file", "blob", oid), f"{name} (objet {oid[:10]})")


def source_file(path):
    if private_path(path):
        return False
    if len(path.parts) == 1:
        return (
            path.name in {".gitignore", "requirements.txt"}
            or path.suffix in {".md", ".yaml", ".yml"}
            or path.name.endswith((".yaml.example", ".yml.example"))
        )
    return path.parts[0] in {"packages", "docs", "systemd", "scripts"} and path.suffix in {
        ".yaml", ".yml", ".md", ".service", ".timer", ".py", ".sh", ".txt"
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Vérifier sans commit ni envoi")
    args = parser.parse_args()
    os.environ["GIT_TERMINAL_PROMPT"] = "0"
    os.environ["GIT_SSH_COMMAND"] = (
        "ssh -o BatchMode=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=15"
    )

    git_dir = Path(git("rev-parse", "--absolute-git-dir").decode().strip())
    with (git_dir / "github-backup.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        branch = git("symbolic-ref", "--short", "HEAD").decode().strip()
        if branch != "master":
            raise SystemExit("Sauvegarde suspendue : revenir sur la branche master.")
        for operation in ("MERGE_HEAD", "CHERRY_PICK_HEAD", "rebase-merge", "rebase-apply"):
            if (git_dir / operation).exists():
                raise SystemExit("Sauvegarde suspendue : opération Git en cours.")

        check_history("HEAD")
        files = sorted({
            os.fsdecode(name)
            for name in git("ls-files", "--cached", "--others", "--exclude-standard", "-z").split(b"\0")
            if name and source_file(Path(os.fsdecode(name)))
        })
        for name in files:
            path = ROOT / name
            if path.suffix in {".yaml", ".yml"} and path.exists():
                check_yaml(path.read_bytes(), name)
        if args.check:
            print("Historique et configurations vérifiés ; aucun commit ni envoi effectué.")
            return
        if git("remote", "get-url", "origin").decode().strip() != REMOTE:
            raise SystemExit("L'adresse origin ne correspond pas au dépôt GitHub attendu.")
        if git("diff", "--cached", "--name-only"):
            raise SystemExit("Des changements sont déjà indexés ; terminer le commit manuel.")
        if files:
            git("add", "--all", "--", *files)
            if git("diff", "--cached", "--name-only"):
                message = f"Back up ESPHome configurations ({datetime.now():%Y-%m-%d %H:%M})"
                print(git(
                    "-c", "user.name=ESPHome Backup",
                    "-c", "user.email=esphome-backup@localhost",
                    "commit", "--only", "-m", message, "--", *files
                ).decode(), end="")
        # Figer le commit contrôlé pour ne pas publier un commit ajouté
        # simultanément par Device Builder après le contrôle.
        revision = git("rev-parse", "HEAD").decode().strip()
        check_history(revision)
        subprocess.run(
            ["git", "-C", str(ROOT), "push", "origin", f"{revision}:refs/heads/master"],
            check=True,
        )
        print(f"Sauvegarde GitHub réussie : {revision[:12]}")


if __name__ == "__main__":
    main()
