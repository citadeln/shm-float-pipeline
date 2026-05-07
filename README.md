# Высокопроизводительный канал сжатых данных



## Сборка проекта
```bash
./scripts/build.sh
```

## Запуск проекта
```bash
./scripts/run.sh
```

## Структура проекта

CompressedShmChannel/
├── CMakeLists.txt
├── README.md
├── build.sh
├── tests/
│   └── test_utils.cpp
├── data/
│   └── sample.bin
├── include/
│   ├── shm_ring.h      # Только IPC (ShmRingBuffer)
│   ├── compressor.h    # FloatQuantizer
│   ├── packet.h        # ChunkHeader
│   └── metrics.h       # Timer/Ratio/Loss
└── src/
    ├── producer.cpp    
    ├── consumer.cpp  
    ├── shm_ring_buffer.cpp
    ├── compressor.cpp
    ├── metrics.cpp
    └── utils.cpp
