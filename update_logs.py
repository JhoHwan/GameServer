import os
import re

def update_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Case 1: LOG_XXX("[Category] ...", ...) -> LOG_XXX(Category, "...", ...)
    # Matches: LOG_INFO("[PathFind] ...") or LOG_INFO("[PathFind] ...", args)
    # We look for the bracketed category at the very beginning of the string.
    pattern1 = r'LOG_(INFO|DEBUG|WARN|ERROR)\(\s*\"\[(\w+)\]\s*(.*?)\"\s*(,.*?)?\)'
    
    def replace1(match):
        level = match.group(1)
        category = match.group(2)
        message = match.group(3)
        args = match.group(4) if match.group(4) else ""
        return f'LOG_{level}({category}, "{message}"{args})'

    new_content = re.sub(pattern1, replace1, content)

    # Case 2: LOG_XXX("...", ...) -> LOG_XXX(Default, "...", ...)
    # Matches: LOG_INFO("...") or LOG_INFO("...", args)
    # But only if it hasn't been processed yet (doesn't have a symbol as the first arg).
    # We look for LOG_XXX( followed by a quote.
    pattern2 = r'LOG_(INFO|DEBUG|WARN|ERROR)\(\s*\"(.*?)\"\s*(,.*?)?\)'
    
    def replace2(match):
        level = match.group(1)
        message = match.group(2)
        args = match.group(3) if match.group(3) else ""
        return f'LOG_{level}(Default, "{message}"{args})'

    new_content = re.sub(pattern2, replace2, new_content)

    if content != new_content:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        return True
    return False

# Directories to search
dirs = [
    r"Server\ServerCore",
    r"Server\GameServer",
    r"Server\DummyServer",
    r"Server\DummyClient"
]

files_updated = 0
for d in dirs:
    if not os.path.exists(d):
        continue
    for root, _, files in os.walk(d):
        for file in files:
            if file.endswith(('.h', '.cpp')):
                if update_file(os.path.join(root, file)):
                    files_updated += 1
                    print(f"Updated: {os.path.join(root, file)}")

print(f"Total files updated: {files_updated}")
