from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
import os

app = FastAPI()

app.mount("/", StaticFiles(directory="resources"), name="resources")

@app.get("/")
def read_root():
    return FileResponse("/index.html")

@app.get("/{file_path:path}")
async def get_file(file_path: str):
    print(file_path)
    file_location = os.path.join("resources", file_path)
    if os.path.isfile(file_location):
        return FileResponse(file_location)
    else:
        raise HTTPException(status_code=404, detail="File not found")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
