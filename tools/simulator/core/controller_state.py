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
class OutletStatus:
    name: str
    state: bool
    current: float = 0.0

@dataclass
class RackStatus:
    name: str
    ip: str
    state: str = "unknown"
    current: float = 0.0
    current_available: bool = False
    online: bool = False
    outlets: List[OutletStatus] = field(default_factory=list)

@dataclass
class GlobalState:
    power: SystemState = SystemState.OFF
    total_current: float = 0.0
    racks: List[RackStatus] = field(default_factory=list)
