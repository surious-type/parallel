import pandas as pd


def build_table(df: pd.DataFrame) -> pd.DataFrame:
    table = df.pivot_table(
        index=["threads", "compiler"],
        columns=["N", "opt"],
        values="time",
        aggfunc="mean"
    )
    
    return table.sort_index()


def make_table_styler():
    count = 1

    def styled_table(table: pd.DataFrame, caption: str) -> pd.DataFrame:
        nonlocal count
        styled = (
        table.style
        .set_caption(f"Таблица {count}. {caption}")
        .format("{:.6f}")
        .set_table_styles([
                {"selector": "caption", "props": [
                    ("caption-side", "top"),
                    ("font-size", "14px"),
                    ("font-style", "italic"),
                    ("text-align", "center"),
                ]},
                {"selector": "th", "props": [
                    ("text-align", "center"),
                    ("font-weight", "bold"),
                    ("border", "1px solid black"),
                ]},
                {"selector": "td", "props": [
                    ("text-align", "center"),
                    ("border", "1px solid black"),
                ]},
                {"selector": "table", "props": [
                    ("border-collapse", "collapse"),
                    ("margin", "0 auto"),
                ]}
            ])
        )
    
        count += 1
        
        return styled

    return styled_table