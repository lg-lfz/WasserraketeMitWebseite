Import("env")

env.AddPreAction("upload", env.VerboseAction(
    "$UPLOADFSIMAGE_CMD", "Uploading Filesystem..."
))