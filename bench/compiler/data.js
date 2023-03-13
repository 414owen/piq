window.BENCHMARK_DATA = {
  "lastUpdate": 1678712575741,
  "repoUrl": "https://github.com/414owen/lang-c",
  "entries": {
    "Language Compiler Metrics": [
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
          "id": "ac845140b7282d385ef6b9a5fd0f8875497c7c65",
          "message": "fix: continuous benchmark branch",
          "timestamp": "2023-03-11T12:07:12+01:00",
          "tree_id": "3089ff184a0aff94d298ea3a016d49b211fd2a96",
          "url": "https://github.com/414owen/lang-c/commit/ac845140b7282d385ef6b9a5fd0f8875497c7c65"
        },
        "date": 1678533044997,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 25958700,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 18.017,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.833,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 42685762,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 59.252,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 88.928,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 205278780,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 427.661,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 52455599,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 807003,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 109.282,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.681,
            "unit": "ns"
          }
        ]
      },
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
          "id": "e3e0ee99ea6bb2229e1490bcfa8c9777c2554ab6",
          "message": "refactor: update compiler benchmark folder",
          "timestamp": "2023-03-11T13:14:45+01:00",
          "tree_id": "e29a7ea3d8eaa2fb8fd513d330fec86d14199b88",
          "url": "https://github.com/414owen/lang-c/commit/e3e0ee99ea6bb2229e1490bcfa8c9777c2554ab6"
        },
        "date": 1678537042081,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22739598,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.782,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.358,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 40502196,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 56.221,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 84.379,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 158656184,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 330.531,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46885396,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 775199,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 97.677,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.615,
            "unit": "ns"
          }
        ]
      },
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
          "id": "23bb62d76f8c5edaadc076a0c1e4440101a9c024",
          "message": "chore: Max out flamegraph samples",
          "timestamp": "2023-03-11T15:55:18+01:00",
          "tree_id": "1252cd83ad14062204adf5595c1a3bdea91fb4ac",
          "url": "https://github.com/414owen/lang-c/commit/23bb62d76f8c5edaadc076a0c1e4440101a9c024"
        },
        "date": 1678546751532,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 30784392,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 21.366,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 4.545,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 46341209,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 64.326,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 96.543,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 205628435,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 428.389,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 58855128,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 1230940,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 122.614,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 2.564,
            "unit": "ns"
          }
        ]
      },
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
          "id": "6b39336055c86ede9f639f0b3ca822ed1069fe53",
          "message": "perf: Fix hashmap growth factor to 2",
          "timestamp": "2023-03-11T16:34:05+01:00",
          "tree_id": "4ae3182b1e3300f7481a19ce47fe9a288e47631c",
          "url": "https://github.com/414owen/lang-c/commit/6b39336055c86ede9f639f0b3ca822ed1069fe53"
        },
        "date": 1678549142194,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22649970,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.72,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.344,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 40951844,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 56.845,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 85.316,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 159998882,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 333.328,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46852036,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 784999,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 97.608,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.635,
            "unit": "ns"
          }
        ]
      },
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
          "id": "bf36e036fc66ece17e17e1c27b56b6eaa79fd64f",
          "message": "perf: Optimize hashmap constants",
          "timestamp": "2023-03-11T17:00:10+01:00",
          "tree_id": "1026b73d48511d30fc77ddb8b418758a512768fe",
          "url": "https://github.com/414owen/lang-c/commit/bf36e036fc66ece17e17e1c27b56b6eaa79fd64f"
        },
        "date": 1678550831875,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22833372,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.848,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.371,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 40335749,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 55.99,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 84.032,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 168267888,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 350.555,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 47235840,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 799299,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 98.407,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.665,
            "unit": "ns"
          }
        ]
      },
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
          "id": "b0957f4409f817995cddfdf82e1ae38df1cdfe40",
          "message": "refactor: Remove unused scope functions",
          "timestamp": "2023-03-11T17:27:15+01:00",
          "tree_id": "3561528aa019eeb105c1e334be8effe234193a5e",
          "url": "https://github.com/414owen/lang-c/commit/b0957f4409f817995cddfdf82e1ae38df1cdfe40"
        },
        "date": 1678553962957,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 23356978,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.211,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.449,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 35883871,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 49.811,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 74.757,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 176705138,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 368.133,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 43573630,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 859106,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 90.778,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.79,
            "unit": "ns"
          }
        ]
      },
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
          "id": "107380910621704be761c31908517cf718b6d90a",
          "message": "refactor: Move all scope resolution to resolve_scope.c",
          "timestamp": "2023-03-11T17:33:47+01:00",
          "tree_id": "190fb65f22a0feabb10fce03779ebfe790046226",
          "url": "https://github.com/414owen/lang-c/commit/107380910621704be761c31908517cf718b6d90a"
        },
        "date": 1678553979044,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22916732,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.905,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.384,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 40262880,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 55.889,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 83.88,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 162917917,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 339.409,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46406263,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 784997,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 96.679,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.635,
            "unit": "ns"
          }
        ]
      },
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
          "id": "1f9bc515380189e8cc4cebbb22dd09d5e9b1abf4",
          "message": "perf: Migrate to closed hash table",
          "timestamp": "2023-03-11T23:54:18+01:00",
          "tree_id": "608380835dc0c1907e38d2473818261947868ebb",
          "url": "https://github.com/414owen/lang-c/commit/1f9bc515380189e8cc4cebbb22dd09d5e9b1abf4"
        },
        "date": 1678575487121,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22653112,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.722,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.345,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 40607801,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 56.368,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 84.599,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 150218943,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 312.954,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46427729,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 748104,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 96.724,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.559,
            "unit": "ns"
          }
        ]
      },
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
          "id": "d8a587c4b5caa64dd77c13add6742d390a064aa5",
          "message": "perf: Try optimizing bitset",
          "timestamp": "2023-03-12T10:30:07+01:00",
          "tree_id": "747a17cd4384a4d1d62604d6081d012837ac3d73",
          "url": "https://github.com/414owen/lang-c/commit/d8a587c4b5caa64dd77c13add6742d390a064aa5"
        },
        "date": 1678613666682,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 30721656,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 21.322,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 4.536,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 51356057,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 71.288,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 106.991,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 215307481,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 448.554,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 57512228,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 1052495,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 119.816,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 2.193,
            "unit": "ns"
          }
        ]
      },
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
          "id": "df4bca7cd449334ba050f56eaa7d04efb810c867",
          "message": "perf: Add restrict to all bitset pointer fns",
          "timestamp": "2023-03-12T10:32:02+01:00",
          "tree_id": "7c44f51381ab2aeef7dc21777a87322b84c1138b",
          "url": "https://github.com/414owen/lang-c/commit/df4bca7cd449334ba050f56eaa7d04efb810c867"
        },
        "date": 1678613777689,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22619020,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.699,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.34,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 41008036,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 56.923,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 85.433,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 149777533,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 312.034,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46206541,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 748501,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 96.263,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.559,
            "unit": "ns"
          }
        ]
      },
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
          "id": "20f8332a3b62471970f5512062e9f4e9b0118a0d",
          "message": "perf: Grow bitsets by 1.5x",
          "timestamp": "2023-03-12T10:46:45+01:00",
          "tree_id": "a566b5835cae5b7ba56cbb51aac7360b1301a615",
          "url": "https://github.com/414owen/lang-c/commit/20f8332a3b62471970f5512062e9f4e9b0118a0d"
        },
        "date": 1678614740129,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 25170396,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 17.47,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.717,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 36348572,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 50.456,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 75.726,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 165422207,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 344.627,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 44036994,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 847713,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 91.743,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.766,
            "unit": "ns"
          }
        ]
      },
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
          "id": "047904875f0c371bb8f1a399545739d66676154d",
          "message": "perf: Grow vectors by 1.5x",
          "timestamp": "2023-03-12T11:39:37+01:00",
          "tree_id": "6201ed5f6a328c9c076e3478d74171f25981e670",
          "url": "https://github.com/414owen/lang-c/commit/047904875f0c371bb8f1a399545739d66676154d"
        },
        "date": 1678618368072,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 23519682,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.324,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.473,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 36984873,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 51.339,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 77.051,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 162731219,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 339.021,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 42720938,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 856925,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 89.001,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.785,
            "unit": "ns"
          }
        ]
      },
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
          "id": "1e241ea218c84c5397f7d8fc222df7e7e06ed198",
          "message": "perf: Restrict traversal arrays",
          "timestamp": "2023-03-12T11:50:26+01:00",
          "tree_id": "482bde6c8b646e1480b0d2326b316ba4782ac361",
          "url": "https://github.com/414owen/lang-c/commit/1e241ea218c84c5397f7d8fc222df7e7e06ed198"
        },
        "date": 1678618478157,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 26440668,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 18.351,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.904,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 50825016,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 70.55,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 105.885,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 176520491,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 367.748,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 52165530,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 988010,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 108.677,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 2.058,
            "unit": "ns"
          }
        ]
      },
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
          "id": "02cc33d9d4ace1a586ad25f2173a5b39b796e0b8",
          "message": "perf: Make traversal bindings const",
          "timestamp": "2023-03-12T11:51:28+01:00",
          "tree_id": "7f03ec879cf72601b87ab4d4dbddd91c665e1aba",
          "url": "https://github.com/414owen/lang-c/commit/02cc33d9d4ace1a586ad25f2173a5b39b796e0b8"
        },
        "date": 1678618539286,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 23159716,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.074,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.42,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 36062495,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 50.059,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 75.13,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 162406829,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 338.345,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 42749387,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 841111,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 89.06,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.752,
            "unit": "ns"
          }
        ]
      },
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
          "id": "408bebb8e32a97bed0796e528d573c5d1499231a",
          "message": "perf: Change optimization flags",
          "timestamp": "2023-03-12T12:28:16+01:00",
          "tree_id": "d43dbc3b4bc088567ff5ab2043ffa732f546b630",
          "url": "https://github.com/414owen/lang-c/commit/408bebb8e32a97bed0796e528d573c5d1499231a"
        },
        "date": 1678620690187,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Time spent tokenizing",
            "value": 22677428,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.739,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.348,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 36263563,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 50.338,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 75.548,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 156424366,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 325.881,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 41694318,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 844808,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 86.862,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.76,
            "unit": "ns"
          }
        ]
      },
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
          "id": "b5cb3753ccdc7d01194e5081389bf1c332cf93ea",
          "message": "feat: Collect file size metrics",
          "timestamp": "2023-03-12T13:49:12+01:00",
          "tree_id": "5b6f2953d8fbcf47f3b99867b77dd0020223f9a7",
          "url": "https://github.com/414owen/lang-c/commit/b5cb3753ccdc7d01194e5081389bf1c332cf93ea"
        },
        "date": 1678625867507,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "92536",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172600",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 22785154,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.814,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.364,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 37280952,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 51.75,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 77.668,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 157695367,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 328.529,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 41388480,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 839305,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 86.225,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 1.749,
            "unit": "ns"
          }
        ]
      },
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
          "id": "1d471e8bc3ccd5499e7fa0ef712d93dba4793d6d",
          "message": "feat: Use fast non-nix CI",
          "timestamp": "2023-03-12T16:50:00+01:00",
          "tree_id": "bae5f09a82f2179df14bfafe18111173abfb6667",
          "url": "https://github.com/414owen/lang-c/commit/1d471e8bc3ccd5499e7fa0ef712d93dba4793d6d"
        },
        "date": 1678636264454,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 22427318,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.566,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.312,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 39409157,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 54.704,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 82.102,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 149676557,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 311.824,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46202332,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 8369870,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 96.254,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 17.437,
            "unit": "ns"
          }
        ]
      },
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
          "id": "ddb2ae1e51605addae06476247a88ab857d66f75",
          "message": "ci: Run test/check workflows on master",
          "timestamp": "2023-03-12T19:50:37+01:00",
          "tree_id": "baa63a9c74987353b1210ad92934b0b9b554ce05",
          "url": "https://github.com/414owen/lang-c/commit/ddb2ae1e51605addae06476247a88ab857d66f75"
        },
        "date": 1678647090019,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 22375046,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 15.529,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.304,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 34701281,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 48.169,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 72.294,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 150497953,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 313.535,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 44108284,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 1615818,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 91.891,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 3.366,
            "unit": "ns"
          }
        ]
      },
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
          "id": "1d471e8bc3ccd5499e7fa0ef712d93dba4793d6d",
          "message": "feat: Use fast non-nix CI",
          "timestamp": "2023-03-12T16:50:00+01:00",
          "tree_id": "bae5f09a82f2179df14bfafe18111173abfb6667",
          "url": "https://github.com/414owen/lang-c/commit/1d471e8bc3ccd5499e7fa0ef712d93dba4793d6d"
        },
        "date": 1678648481063,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 24440546,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.963,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.609,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 41947306,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 58.227,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 87.389,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 152384958,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 317.466,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46755595,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 10416077,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 97.407,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 21.7,
            "unit": "ns"
          }
        ]
      },
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
          "id": "7ab237d18f30dba43dc07ad631596c57ef4f8e7b",
          "message": "fix: Don't fail CI on performance regressions",
          "timestamp": "2023-03-12T20:16:38+01:00",
          "tree_id": "ffc9a71d0c3624781b6f45225f6e6a55c42bf8f2",
          "url": "https://github.com/414owen/lang-c/commit/7ab237d18f30dba43dc07ad631596c57ef4f8e7b"
        },
        "date": 1678648638574,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 30975672,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 21.499,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 4.574,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 54683932,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 75.907,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 113.924,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 154391350,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 321.646,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 44592479,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 1895029,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 92.9,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 3.948,
            "unit": "ns"
          }
        ]
      },
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
          "id": "4763b3c296beafec3eb744ce40164322668d6573",
          "message": "fix: Benchmark metric name typo",
          "timestamp": "2023-03-12T20:21:03+01:00",
          "tree_id": "f33e0ef8cf64b2de751f44f5009eb5ba36cd217e",
          "url": "https://github.com/414owen/lang-c/commit/4763b3c296beafec3eb744ce40164322668d6573"
        },
        "date": 1678648933454,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 23360136,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.213,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.449,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 36471224,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 50.626,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 75.981,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 155956143,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 324.906,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 48788001,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 10064045,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 101.641,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 20.967,
            "unit": "ns"
          }
        ]
      },
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
          "id": "05e7c56310ef3aad56f3a7b6ca3bdb1c85ca137b",
          "message": "feat: Link to benchmarks from README",
          "timestamp": "2023-03-13T10:13:34+01:00",
          "tree_id": "11cca4a23f73f0f888b0eb75a84659cb01e39096",
          "url": "https://github.com/414owen/lang-c/commit/05e7c56310ef3aad56f3a7b6ca3bdb1c85ca137b"
        },
        "date": 1678698922428,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 24275180,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.848,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.584,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 39967937,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 55.48,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 83.266,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 152450917,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 317.603,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 46771675,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 40132036,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 97.44,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 83.608,
            "unit": "ns"
          }
        ]
      },
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
          "distinct": false,
          "id": "c6291140c54afa6c3d5d0cfabaf7ad3758f6e304",
          "message": "fix: Actually codegen benchmark",
          "timestamp": "2023-03-13T13:59:14+01:00",
          "tree_id": "fc329bdea47cc127e492188845dbd2f4a3dff31a",
          "url": "https://github.com/414owen/lang-c/commit/c6291140c54afa6c3d5d0cfabaf7ad3758f6e304"
        },
        "date": 1678712574224,
        "tool": "customSmallerIsBetter",
        "benches": [
          {
            "name": "Main filesize",
            "value": "96800",
            "unit": "bytes"
          },
          {
            "name": "Test filesize",
            "value": "172768",
            "unit": "bytes"
          },
          {
            "name": "Time spent tokenizing",
            "value": 24453532,
            "unit": "ns"
          },
          {
            "name": "Total bytes tokenized",
            "value": 6772532,
            "unit": "bytes"
          },
          {
            "name": "Total tokens produced",
            "value": 1440814,
            "unit": "amount"
          },
          {
            "name": "Tokenization time per token produced",
            "value": 16.972,
            "unit": "ns"
          },
          {
            "name": "Tokenization time per byte",
            "value": 3.611,
            "unit": "ns"
          },
          {
            "name": "Time spent parsing",
            "value": 44309436,
            "unit": "ns"
          },
          {
            "name": "Total tokens parsed",
            "value": 720407,
            "unit": "amount"
          },
          {
            "name": "Total parse nodes produced",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Parse nodes produced per token",
            "value": 0.666,
            "unit": "amount"
          },
          {
            "name": "Parse time per token",
            "value": 61.506,
            "unit": "ns"
          },
          {
            "name": "Parse time per parse node produced",
            "value": 92.311,
            "unit": "ns"
          },
          {
            "name": "Time spent typechecking",
            "value": 179190057,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes typechecked",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Typecheck time per parse node",
            "value": 373.31,
            "unit": "ns"
          },
          {
            "name": "Time spent building LLVM IR",
            "value": 49372863,
            "unit": "ns"
          },
          {
            "name": "Time spent performing codegen",
            "value": 180554764,
            "unit": "ns"
          },
          {
            "name": "Total parse nodes turned into LLVM IR",
            "value": 480004,
            "unit": "amount"
          },
          {
            "name": "Time building LLVM IR per parse node",
            "value": 102.859,
            "unit": "ns"
          },
          {
            "name": "Codegen time per parse node",
            "value": 376.153,
            "unit": "ns"
          }
        ]
      }
    ]
  }
}