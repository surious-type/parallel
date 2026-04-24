#!/usr/bin/env python3
import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

from utils import sanitize_filename
from data import aggregate_times, load_data, unique_threads, unique_opts, unique_n


REQUIRED_COLUMNS = {"source", "compiler", "opt", "N", "threads", "time"}


def parse_args():
    parser = argparse.ArgumentParser(
        description="Строит графики ускорения и 3D-графики времени из CSV benchmark-измерений."
    )
    parser.add_argument("csv", type=Path, help="Путь к входному CSV-файлу")
    return parser.parse_args()


def get_output_dir(csv_path: Path) -> Path:
    return csv_path.parent / f"{csv_path.stem}_report"


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


from typing import Tuple
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def make_plot():
    count = 1

    def plot_speedup_by_n_figure(
    group_df: pd.DataFrame,
    source: str,
    compiler: str,
    ) -> Tuple[plt.Figure, np.ndarray]:
        nonlocal count

        n_values = unique_n(group_df)
        opts = unique_opts(group_df)
        threads_all = unique_threads(group_df)
    
        fig, axes = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)
        axes = axes.flatten()
    
        y_max = float(group_df["speedup"].max())
        x_pos = np.arange(len(threads_all))
    
        for ax, opt in zip(axes, opts):
            opt_df = group_df[group_df["opt"] == opt]
    
            ax.plot(
                x_pos,
                threads_all,
                "--",
                color="gray",
                linewidth=1.2,
                alpha=0.5,
                label="_nolegend_",
            )
    
            for n_value in n_values:
                curve = (
                    opt_df[opt_df["N"] == n_value]
                    .sort_values("threads")
                    .set_index("threads")
                    .reindex(threads_all)
                    .reset_index()
                )
    
                ax.plot(
                    x_pos,
                    curve["speedup"],
                    marker="o",
                    linewidth=1.8,
                    label=f"N = {n_value}",
                )
    
            ax.set_xticks(x_pos)
            ax.set_xticklabels(threads_all)
            ax.set_title(f"Оптимизация {opt}")
            ax.set_xlabel("Число нитей")
            ax.set_ylabel("Ускорение")
            ax.set_ylim(0, y_max * 1.08 if y_max > 0 else 1.0)
            ax.grid(True, which="major", alpha=0.35)
            ax.grid(True, which="minor", alpha=0.15)
            ax.legend()
    
        for index in range(len(opts), len(axes)):
            axes[index].axis("off")
    
        fig.suptitle(
            f"График {count}. Ускорение от размерности данных. Исходник: {source}. Компилятор: {compiler}",
            fontsize=14,
        )

        count += 1
    
        return fig, axes
    
    
    def plot_speedup_by_opt_figure(
        group_df: pd.DataFrame,
        source: str,
        compiler: str,
    ) -> Tuple[plt.Figure, np.ndarray]:
        nonlocal count

        n_values = unique_n(group_df)
        opts = unique_opts(group_df)
        threads_all = unique_threads(group_df)
    
        fig, axes = plt.subplots(2, 2, figsize=(14, 10), constrained_layout=True)
        axes = axes.flatten()
    
        y_max = float(group_df["speedup"].max())
        x_pos = np.arange(len(threads_all))
    
        for ax, n_value in zip(axes, n_values):
            n_df = group_df[group_df["N"] == n_value]

            ax.plot(
                x_pos,
                threads_all,
                "--",
                color="gray",
                linewidth=1.2,
                alpha=0.5,
                label="_nolegend_",
            )
    
            for opt in opts:
                curve = (
                    n_df[n_df["opt"] == opt]
                    .sort_values("threads")
                    .set_index("threads")
                    .reindex(threads_all)
                    .reset_index()
                )
    
                ax.plot(
                    x_pos,
                    curve["speedup"],
                    marker="o",
                    linewidth=1.8,
                    label=f"{opt}",
                )
    
            ax.set_xticks(x_pos)
            ax.set_xticklabels(threads_all)  
    
            ax.set_title(f"N = {n_value}")
            ax.set_xlabel("Число нитей")
            ax.set_ylabel("Ускорение")
            ax.set_ylim(0, y_max * 1.08 if y_max > 0 else 1.0)
            ax.grid(True, alpha=0.3)
            ax.legend()
    
        for index in range(len(n_values), len(axes)):
            axes[index].axis("off")
    
        fig.suptitle(
            f"График {count}. Ускорение от оптимизации. Исходник: {source}. Компилятор: {compiler}",
            fontsize=14,
        )

        count += 1
    
        return fig, axes
    
    
    def plot_3d_figure(
        group_df: pd.DataFrame, 
        source: str,
        compiler: str,
    ) -> Tuple[plt.Figure, np.ndarray]:
        nonlocal count

        opts = unique_opts(group_df)
    
        fig, axes = plt.subplots(2, 2, figsize=(18, 22),
            subplot_kw={"projection": "3d"},
        )
        axes = axes.flatten()
    
        threads_all = unique_threads(group_df)

        for ax, opt in zip(axes, opts):
            opt_df = group_df[group_df["opt"] == opt].sort_values(["N", "threads"])
        
            pivot = opt_df.pivot_table(
                index="N",
                columns="threads",
                values="time",
                aggfunc="mean"
            )
        
            pivot = pivot.reindex(columns=threads_all)
        
            x_values = np.arange(len(threads_all))  # категории
            y_values = pivot.index.to_numpy()
        
            x_grid, y_grid = np.meshgrid(x_values, y_values)
            z_grid = pivot.to_numpy()
        
            ax.plot_surface(x_grid, y_grid, z_grid, alpha=0.9)
        
            ax.set_xticks(x_values)
            ax.set_xticklabels(threads_all)
        
            ax.set_title(f"Оптимизация {opt}")
            ax.set_xlabel("Число нитей")
            ax.set_ylabel("N")
            ax.set_zlabel("Время")
    
        for index in range(len(opts), len(axes)):
            axes[index].axis("off")
    
        fig.suptitle(
            f"График {count}. 3D по времени. Исходник: {source}. Компилятор: {compiler}",
            fontsize=14,
        )

        count += 1

        return fig, axes

    return plot_speedup_by_n_figure, plot_speedup_by_opt_figure, plot_3d_figure


def build_plots(speedup_df: pd.DataFrame, out_dir: Path) -> list[Path]:
    created_files = []
    dpi = 200

    for (source, compiler), group_df in speedup_df.groupby(["source", "compiler"]):
        suffix = f"{sanitize_filename(source)}__{sanitize_filename(compiler)}"

        speedup_path = out_dir / f"speedup_n_{suffix}.png"
        fig = plot_speedup_by_n_figure(group_df, source, compiler)
        fig.savefig(speedup_path, dpi=dpi, bbox_inches="tight")
        plt.close(fig)
        created_files.append(speedup_path)

        speedup_path = out_dir / f"speedup_opt_{suffix}.png"
        fig = plot_speedup_by_opt_figure(group_df, source, compiler)
        fig.savefig(speedup_path, dpi=dpi, bbox_inches="tight")
        plt.close(fig)
        created_files.append(speedup_path)

        plot3d_path = out_dir / f"surface3d_{suffix}.png"
        fig = plot_3d_figure(group_df, source, compiler)
        fig.savefig(plot3d_path, dpi=dpi, bbox_inches="tight")
        plt.close(fig)
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
