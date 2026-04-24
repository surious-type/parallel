import pandas as pd
from pathlib import Path

REQUIRED_COLUMNS = {"source", "compiler", "opt", "N", "threads", "time"}

def load_data(csv_path: Path) -> pd.DataFrame:
    df = pd.read_csv(csv_path)

    missing = REQUIRED_COLUMNS - set(df.columns)
    if missing:
        missing_str = ", ".join(sorted(missing))
        raise ValueError(f"В CSV отсутствуют обязательные колонки: {missing_str}")

    df = df.copy()
    df["N"] = pd.to_numeric(df["N"], errors="coerce")
    df["threads"] = pd.to_numeric(df["threads"], errors="coerce")
    df["time"] = pd.to_numeric(df["time"], errors="coerce")

    df = df.dropna(subset=["source", "compiler", "opt", "N", "threads", "time"]).copy()
    df["N"] = df["N"].astype(int)
    df["threads"] = df["threads"].astype(int)

    df["opt"] = df["opt"].replace({
        "fast": "Ofast"
    })

    return df


def select_data(
    df,
    source,
):
    out = df.copy()
    out = out[out["source"] == source]

    return out


def aggregate_times(df: pd.DataFrame) -> pd.DataFrame:
    return (
        df.groupby(["source", "compiler", "opt", "N", "threads"], as_index=False)[
            "time"
        ]
        .mean()
        .sort_values(["source", "compiler", "opt", "N", "threads"])
        .reset_index(drop=True)
    )


def unique_n(df: pd.DataFrame):
    return sorted(df["N"].unique())


def unique_opts(df: pd.DataFrame):
    return sorted(df["opt"].unique())


def unique_threads(df: pd.DataFrame):
    return sorted(df["threads"].unique())


def add_speedup(df: pd.DataFrame) -> pd.DataFrame:
    baseline = (
        df[df["threads"] == 1][["source", "compiler", "opt", "N", "time"]]
        .rename(columns={"time": "time_1_thread"})
        .drop_duplicates(subset=["source", "compiler", "opt", "N"])
    )

    result = df.merge(
        baseline,
        on=["source", "compiler", "opt", "N"],
        how="left",
    )

    result = result.dropna(subset=["time_1_thread"]).copy()
    result["speedup"] = result["time_1_thread"] / result["time"]

    return result