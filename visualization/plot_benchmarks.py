import sys
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


def sanitize_name(name: str) -> str:
    bad = '<>:"/\\|?*'
    for ch in bad:
        name = name.replace(ch, "_")
    return name.strip().replace(" ", "_")


def find_csv_files(input_path: Path):
    if input_path.is_file():
        return [input_path]
    return sorted(p for p in input_path.glob("*.csv") if p.is_file())


def load_csv(csv_path: Path) -> pd.DataFrame:
    df = pd.read_csv(csv_path)

    required = {"threads", "time"}
    missing = required - set(df.columns)
    if missing:
        raise ValueError(
            f"{csv_path.name}: отсутствуют обязательные колонки: {', '.join(sorted(missing))}"
        )

    df = df.copy()
    df["threads"] = pd.to_numeric(df["threads"], errors="coerce")
    df["time"] = pd.to_numeric(df["time"], errors="coerce")

    df = df.dropna(subset=["threads", "time"])
    df = df[df["threads"] >= 1]

    if df.empty:
        raise ValueError(
            f"{csv_path.name}: после фильтрации не осталось валидных строк"
        )

    return df


def build_summary(df: pd.DataFrame) -> pd.DataFrame:
    summary = (
        df.groupby("threads", as_index=False)["time"]
        .mean()
        .rename(columns={"time": "mean_time"})
        .sort_values("threads")
    )

    base_row = summary[summary["threads"] == 1]
    if base_row.empty:
        raise ValueError("Нет данных для threads=1, невозможно вычислить ускорение")

    t1 = base_row["mean_time"].iloc[0]
    print(t1)
    summary["speedup"] = t1 / summary["mean_time"]

    return summary


def plot_runtime(
    df: pd.DataFrame, summary: pd.DataFrame, title: str, out_png: Path
) -> None:
    plt.figure(figsize=(10, 6))

    plt.scatter(df["threads"], df["time"], alpha=0.5, label="Отдельные прогоны")
    plt.plot(
        summary["threads"], summary["mean_time"], marker="o", label="Среднее время"
    )

    plt.xlabel("Число потоков")
    plt.ylabel("Время, сек")
    plt.title(f"Время выполнения\n{title}")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_png, dpi=160)
    plt.close()


def plot_speedup(summary: pd.DataFrame, title: str, out_png: Path) -> None:
    plt.figure(figsize=(10, 6))

    plt.plot(
        summary["threads"], summary["speedup"], marker="o", label="Измеренное ускорение"
    )
    plt.plot(
        summary["threads"],
        summary["threads"],
        linestyle="--",
        label="Идеальное ускорение",
    )

    plt.xlabel("Число потоков")
    plt.ylabel("Ускорение")
    plt.title(f"Ускорение\n{title}")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_png, dpi=160)
    plt.close()


def process_csv(csv_path: Path, output_dir: Path) -> None:
    df = load_csv(csv_path)
    safe_stem = sanitize_name(csv_path.stem)
    title = safe_stem
    summary = build_summary(df)

    runtime_png = output_dir / f"{safe_stem}_runtime.png"
    speedup_png = output_dir / f"{safe_stem}_speedup.png"

    plot_runtime(df, summary, title, runtime_png)
    plot_speedup(summary, title, speedup_png)

    print(f"[OK] {csv_path.name}")
    print(f"     runtime: {runtime_png}")
    print(f"     speedup: {speedup_png}")


def main():
    input_path = Path("results")
    output_dir = Path("plots")
    output_dir.mkdir(parents=True, exist_ok=True)

    if not input_path.exists():
        print(f"Ошибка: путь не найден: {input_path}", file=sys.stderr)
        sys.exit(1)

    csv_files = find_csv_files(input_path)
    if not csv_files:
        print("Ошибка: CSV-файлы не найдены", file=sys.stderr)
        sys.exit(1)

    failed = []
    for csv_path in csv_files:
        try:
            process_csv(csv_path, output_dir)
        except Exception as e:
            failed.append((csv_path.name, str(e)))
            print(f"[FAIL] {csv_path.name}: {e}", file=sys.stderr)

    if failed:
        print("\nНе удалось обработать:", file=sys.stderr)
        for name, err in failed:
            print(f"  - {name}: {err}", file=sys.stderr)
        sys.exit(2)


if __name__ == "__main__":
    main()
