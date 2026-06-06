from enum import Enum
from dataclasses import dataclass, field
from typing import List, Optional

class SystemState(Enum):
    OFF = "off"
    ON = "on"
    MIXED = "mixed"
    ERROR = "error"
    SEQUENCING = "sequencing"

@dataclass
class RackStatus:
    name: str
    ip: str
    state: str = "unknown"
    current: float = 0.0
    current_available: bool = False
    online: bool = False

@dataclass
class GlobalState:
    power: SystemState = SystemState.OFF
    total_current: float = 0.0
    racks: List[RackStatus] = field(default_factory=list)
