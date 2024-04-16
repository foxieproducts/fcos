file = open("release.md")
print("-D FW_VERSION=\"%s\"" % file.readline())