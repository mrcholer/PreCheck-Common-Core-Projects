from dataclasses import dataclass, field

FAILING = {"FAIL", "CRASH", "TIMEOUT", "LEAK", "MEMORY"}


@dataclass
class Result:
    func: str
    cat: str
    name: str
    status: str          # PASS FAIL WARN CRASH TIMEOUT LEAK MEMORY INFO SKIP MISSING
    tier: int = 1
    call: str = ""
    expected: str = ""
    actual: str = ""
    hint: str = ""

    @property
    def failing(self):
        return self.status in FAILING

    def to_dict(self):
        return self.__dict__.copy()


def project_result(name, status, expected="", actual="", hint=""):
    return Result("(project)", "project", name, status, 1, "", expected, actual, hint)
