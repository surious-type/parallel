#!/usr/bin/env python3
import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


REQUIRED_COLUMNS = {"source", "compiler", "opt", "N", "threads", "time"}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Строит графики ускорения и 3D-графики времени из CSV benchmark-измерений."
    )
    parser.add_argument("csv", type=Path, help="Путь к входному CSV-файлу")
    return parser.parse_args()


def sanitize_filename(value: str) -> str:
    bad = '<>:"/\\|?* '
    result = str(value)
    for ch in bad:
        result = result.replace(ch, "_")
    return result.strip("_")


def get_output_dir(csv_path: Path) -> Path:
    return csv_path.parent / f"{csv_path.stem}_report"


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

    return df


def aggregate_times(df: pd.DataFrame) -> pd.DataFrame:
    return (
        df.groupby(["source", "compiler", "opt", "N", "threads"], as_index=False)[
            "time"
        ]
        .mean()
        .sort_values(["source", "compiler", "opt", "N", "threads"])
        .reset_index(drop=True)
    )


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


def build_summary(speedup_df: pd.DataFrame) -> pd.DataFrame:
    rows = []

    for keys, group in speedup_df.groupby(["source", "compiler", "opt", "N"]):
        source, compiler, opt, n_value = keys
        best_row = group.loc[group["speedup"].idxmax()]

        rows.append(
            {
                "source": source,
                "compiler": compiler,
                "opt": opt,
                "N": n_value,
                "avg_time": group["time"].mean(),
                "best_time": group["time"].min(),
                "best_threads": int(best_row["threads"]),
                "best_speedup": best_row["speedup"],
            }
        )

    summary_df = pd.DataFrame(rows)
    return summary_df.sort_values(["source", "compiler", "opt", "N"]).reset_index(
        drop=True
    )


def save_summary(summary_df: pd.DataFrame, out_dir: Path) -> Path:
    out_path = out_dir / "summary.csv"
    summary_df.to_csv(out_path, index=False, float_format="%.6f")
    return out_path


def plot_speedup_figure(group_df: pd.DataFrame, out_path: Path) -> None:
    source = group_df["source"].iloc[0]
    compiler = group_df["compiler"].iloc[0]

    opts = sorted(group_df["opt"].unique())
    fig, axes = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)
    axes = axes.flatten()

    for ax, opt in zip(axes, opts):
        opt_df = group_df[group_df["opt"] == opt]

        for n_value in sorted(opt_df["N"].unique()):
            curve = opt_df[opt_df["N"] == n_value].sort_values("threads")
            ax.plot(
                curve["threads"],
                curve["speedup"],
                marker="o",
                linewidth=1.8,
                label=f"N = {n_value}",
            )

        ax.set_title(f"Оптимизация {opt}")
        ax.set_xlabel("Число нитей")
        ax.set_ylabel("Ускорение")
        ax.grid(True, alpha=0.3)
        ax.legend()

    for index in range(len(opts), len(axes)):
        axes[index].axis("off")

    fig.suptitle(
        f"Графики ускорения: source={source}, compiler={compiler}",
        fontsize=14,
    )
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def plot_3d_figure(group_df: pd.DataFrame, out_path: Path) -> None:
    source = group_df["source"].iloc[0]
    compiler = group_df["compiler"].iloc[0]

    opts = sorted(group_df["opt"].unique())
    fig = plt.figure(figsize=(14, 10), constrained_layout=True)

    for position, opt in enumerate(opts, start=1):
        ax = fig.add_subplot(2, 2, position, projection="3d")
        opt_df = group_df[group_df["opt"] == opt].sort_values(["N", "threads"])

        pivot = opt_df.pivot(index="N", columns="threads", values="time")
        x_values = pivot.columns.to_numpy()
        y_values = pivot.index.to_numpy()
        x_grid, y_grid = np.meshgrid(x_values, y_values)
        z_grid = pivot.to_numpy()

        ax.plot_surface(x_grid, y_grid, z_grid, alpha=0.9)
        ax.set_title(f"Оптимизация {opt}")
        ax.set_xlabel("Число нитей")
        ax.set_ylabel("N")
        ax.set_zlabel("Время")

    for position in range(len(opts) + 1, 5):
        ax = fig.add_subplot(2, 2, position, projection="3d")
        ax.set_axis_off()

    fig.suptitle(
        f"3D-графики времени: source={source}, compiler={compiler}",
        fontsize=14,
    )
    fig.savefig(out_path, dpi=200)
    plt.close(fig)


def build_plots(speedup_df: pd.DataFrame, out_dir: Path) -> list[Path]:
    created_files = []

    for (source, compiler), group_df in speedup_df.groupby(["source", "compiler"]):
        suffix = f"{sanitize_filename(source)}__{sanitize_filename(compiler)}"

        speedup_path = out_dir / f"speedup_{suffix}.png"
        plot_speedup_figure(group_df, speedup_path)
        created_files.append(speedup_path)

        plot3d_path = out_dir / f"surface3d_{suffix}.png"
        plot_3d_figure(group_df, plot3d_path)
        created_files.append(plot3d_path)

    return created_files


def main():
    args = parse_args()
    csv_path = args.csv.resolve()
    out_dir = get_output_dir(csv_path)
    out_dir.mkdir(parents=True, exist_ok=True)

    raw_df = load_data(csv_path)
    agg_df = aggregate_times(raw_df)
    speedup_df = add_speedup(agg_df)

    summary_df = build_summary(speedup_df)
    summary_path = save_summary(summary_df, out_dir)
    plot_files = build_plots(speedup_df, out_dir)

    print(f"Папка с результатами: {out_dir}")
    print(f"summary.csv: {summary_path}")
    for path in plot_files:
        print(path)


if __name__ == "__main__":
    main()
