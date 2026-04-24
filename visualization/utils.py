def sanitize_filename(value: str) -> str:
    bad = '<>:"/\\|?* '
    result = str(value)
    for ch in bad:
        result = result.replace(ch, "_")
    return result.strip("_")