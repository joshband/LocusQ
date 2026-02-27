from __future__ import annotations
import json
from dataclasses import dataclass
from typing import Dict, List, Tuple
import numpy as np
from .ear_embed import cosine

@dataclass(frozen=True)
class SubjectEmbeddings:
    subject_id: str
    left: np.ndarray
    right: np.ndarray

def load_embeddings_npz(path: str) -> List[SubjectEmbeddings]:
    data = np.load(path, allow_pickle=True)
    subjects = []
    for sid in data["subject_ids"]:
        left = data[f"{sid}_left"]
        right = data[f"{sid}_right"]
        subjects.append(SubjectEmbeddings(subject_id=str(sid), left=left, right=right))
    return subjects

def score_subject(user_left: np.ndarray, user_right: np.ndarray, subj: SubjectEmbeddings) -> float:
    return 0.5 * (cosine(user_left, subj.left) + cosine(user_right, subj.right))

def select_topk(user_left: np.ndarray, user_right: np.ndarray, subjects: List[SubjectEmbeddings], k: int = 5):
    scored = [(s.subject_id, score_subject(user_left, user_right, s)) for s in subjects]
    scored.sort(key=lambda t: t[1], reverse=True)
    return scored[:k]

def write_user_profile(path: str, dataset: str, embedding_model: str, topk: List[Tuple[str, float]]):
    profile = {
        "version": "1",
        "dataset": dataset,
        "selection_method": "ear_photo_nearest_neighbor",
        "embedding_model": embedding_model,
        "selected_subject_id": topk[0][0],
        "topk": [{"subject_id": sid, "score": float(score)} for sid, score in topk],
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(profile, f, indent=2)
