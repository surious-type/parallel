import numpy as np
import matplotlib.pyplot as plt
import imageio.v2 as imageio
from pathlib import Path

# -----------------------------
# Параметры задачи
# -----------------------------
N = 2 * 2 * 2 * 2 * 2 * 2 + 2   # 66
maxeps = 0.1e-7
itmax = 100

output_dir = Path("frames_relax")
output_dir.mkdir(exist_ok=True)

gif_name = "relax_iterations.gif"

# -----------------------------
# Инициализация массива
# -----------------------------
def init_array(N):
    A = np.zeros((N, N, N), dtype=np.float32)

    for k in range(N):
        for j in range(N):
            for i in range(N):
                if i == 0 or i == N-1 or j == 0 or j == N-1 or k == 0 or k == N-1:
                    A[i, j, k] = 0.0
                else:
                    A[i, j, k] = 1.0 + i + j + k
    return A

# -----------------------------
# Одна итерация relax()
# полностью по логике C-кода
# -----------------------------
def relax(A):
    eps = 0.0
    N = A.shape[0]

    # 1-й проход по k
    for k in range(3, N - 3):
        for j in range(1, N - 1):
            for i in range(1, N - 1):
                A[i, j, k] = (
                    A[i, j, k - 1] + A[i, j, k + 1] +
                    A[i, j, k - 2] + A[i, j, k + 2] +
                    A[i, j, k - 3] + A[i, j, k + 3]
                ) / 6.0

    # 2-й проход по i
    for k in range(1, N - 1):
        for j in range(1, N - 1):
            for i in range(3, N - 3):
                A[i, j, k] = (
                    A[i - 1, j, k] + A[i + 1, j, k] +
                    A[i - 2, j, k] + A[i + 2, j, k] +
                    A[i - 3, j, k] + A[i + 3, j, k]
                ) / 6.0

    # 3-й проход по j + вычисление eps
    for k in range(1, N - 1):
        for j in range(3, N - 3):
            for i in range(1, N - 1):
                e = A[i, j, k]
                A[i, j, k] = (
                    A[i, j - 1, k] + A[i, j + 1, k] +
                    A[i, j - 2, k] + A[i, j + 2, k] +
                    A[i, j - 3, k] + A[i, j + 3, k]
                ) / 6.0
                eps = max(eps, abs(e - A[i, j, k]))

    return eps

# -----------------------------
# Проверка, как в C-коде
# -----------------------------
def verify(A):
    N = A.shape[0]
    s = 0.0
    for k in range(N):
        for j in range(N):
            for i in range(N):
                s += A[i, j, k] * (i + 1) * (j + 1) * (k + 1) / (N * N * N)
    return s

# -----------------------------
# Сохранение одного кадра
# -----------------------------
def save_frame(A, iteration, eps, vmin, vmax, out_path):
    mid = A.shape[0] // 2

    # Центральные срезы
    slice_xy = A[:, :, mid]
    slice_xz = A[:, mid, :]
    slice_yz = A[mid, :, :]

    fig, axes = plt.subplots(1, 3, figsize=(15, 5), constrained_layout=True)

    images = [
        (slice_xy.T, "Срез XY (k = центр)"),
        (slice_xz.T, "Срез XZ (j = центр)"),
        (slice_yz.T, "Срез YZ (i = центр)")
    ]

    for ax, (data, title) in zip(axes, images):
        im = ax.imshow(
            data,
            origin="lower",
            cmap="viridis",
            vmin=vmin,
            vmax=vmax,
            interpolation="nearest",
            aspect="equal"
        )
        ax.set_title(title)
        ax.set_xlabel("индекс 1")
        ax.set_ylabel("индекс 2")

    fig.suptitle(f"Итерация {iteration}, eps = {eps:.8e}", fontsize=14)
    cbar = fig.colorbar(im, ax=axes.ravel().tolist(), shrink=0.85)
    cbar.set_label("Значение A")

    plt.savefig(out_path, dpi=120)
    plt.close(fig)

# -----------------------------
# Основной запуск
# -----------------------------
def main():
    A = init_array(N)

    # Фиксируем общий диапазон цветов,
    # чтобы GIF не "прыгал" по шкале
    initial_min = np.min(A)
    initial_max = np.max(A)
    vmin = initial_min
    vmax = initial_max

    frame_paths = []

    # Кадр 0: начальное состояние
    frame0 = output_dir / "frame_000.png"
    save_frame(A, 0, 0.0, vmin, vmax, frame0)
    frame_paths.append(frame0)

    for it in range(1, itmax + 1):
        eps = relax(A)
        print(f"it={it:4d}   eps={eps:.10f}")

        frame_path = output_dir / f"frame_{it:03d}.png"
        save_frame(A, it, eps, vmin, vmax, frame_path)
        frame_paths.append(frame_path)

        if eps < maxeps:
            break

    s = verify(A)
    print(f"\nS = {s:.6f}")

    # Сборка GIF
    images = [imageio.imread(path) for path in frame_paths]
    imageio.mimsave(gif_name, images, duration=0.5, loop=0)

    print(f"\nGIF сохранён в файл: {gif_name}")

if __name__ == "__main__":
    main()
