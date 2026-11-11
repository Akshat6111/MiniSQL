from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

app = FastAPI(title="MiniSQL API")

# Allow React frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:8080", "http://localhost:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

class QueryRequest(BaseModel):
    sql: str


@app.get("/")
def root():
    return {"message": "MiniSQL API Running"}


@app.post("/query")
def execute_query(req: QueryRequest):
    print("Received SQL:", req.sql)

    return {
        "success": True,
        "columns": ["id", "name", "age"],
        "rows": [
            {
                "id": 1,
                "name": "Akshat",
                "age": 20,
            },
            {
                "id": 2,
                "name": "Alice",
                "age": 25,
            },
            {
                "id": 3,
                "name": "Bob",
                "age": 30,
            },
        ],
        "used_index": True,
        "rows_examined": 3,
    }


@app.get("/tables")
def tables():
    return {
        "tables": [
            "users",
            "orders",
        ]
    }


@app.get("/schema/{table}")
def schema(table: str):
    return {
        "table": table,
        "columns": [
            {
                "name": "id",
                "type": "INT",
            },
            {
                "name": "name",
                "type": "VARCHAR",
            },
            {
                "name": "age",
                "type": "INT",
            },
        ],
    }


@app.get("/preview/{table}")
def preview(table: str):
    return {
        "columns": ["id", "name", "age"],
        "rows": [
            {
                "id": 1,
                "name": "Akshat",
                "age": 20,
            },
            {
                "id": 2,
                "name": "Alice",
                "age": 25,
            },
        ],
    }


@app.get("/stats")
def stats():
    return {
        "tables": 2,
        "rows": 8,
        "indexes": 2,
        "database_size": "32 KB",
    }


@app.get("/benchmark")
def benchmark():
    return {
        "point_lookup_us": 18,
        "range_lookup_us": 34,
        "speedup": "193x",
    }