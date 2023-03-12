window.BENCHMARK_DATA = {
  "lastUpdate": 1678613778446,
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
            "name": "Parse time per token token",
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
      }
    ]
  }
}