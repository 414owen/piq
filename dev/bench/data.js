window.BENCHMARK_DATA = {
  "lastUpdate": 1676674607926,
  "repoUrl": "https://github.com/414owen/lang-c",
  "entries": {
    "Language Compiler Benchmark": [
      {
        "commit": {
          "author": {
            "email": "owen@owen.cafe",
            "name": "Owen Shepherd",
            "username": "414owen"
          },
          "committer": {
            "email": "owen@owen.cafe",
            "name": "Owen Shepherd",
            "username": "414owen"
          },
          "distinct": true,
          "id": "4a201aeed18f6450fbb200e7fa6a1827e72658a8",
          "message": "chore: test ci",
          "timestamp": "2023-02-17T23:53:18+01:00",
          "tree_id": "2a2fddc380e76fc4c7e8ccbc3e0981581c16f6f4",
          "url": "https://github.com/414owen/lang-c/commit/4a201aeed18f6450fbb200e7fa6a1827e72658a8"
        },
        "date": 1676674606774,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 5659746,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 1590706,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 362014,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.634,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.558,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 10203403,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 181007,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 120004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.663,
            "unit": "amount"
          },
          {
            "name": "Parse time per token token",
            "value": 56.37,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 85.026,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 2609142589,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 120004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 21742.13,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 12162384,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 1748583,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 120004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 101.35,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 14.571,
            "unit": "ns"
          }
        ]
      }
    ]
  }
}