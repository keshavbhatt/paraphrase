from paraeval.daemon import to_result


def _call(ttft: float, total: float) -> dict[str, float]:
    return {
        "ttft_s": ttft,
        "total_s": total,
        "prompt_tokens": 200,
        "cached_tokens": 150,
        "generated_tokens": 20,
        "draft_tokens": 9,
        "draft_accepted": 7,
    }


def test_client_timings_replace_the_engines() -> None:
    line = {
        "output": "They are going home.",
        "finish": "stop",
        "retried": True,
        "warning": "",
        "calls": [_call(0.3, 1.0), _call(0.2, 0.8)],
        "client_ttft_s": 0.35,
        "client_total_s": 1.9,
    }
    r = to_result(line)
    assert r.output == "They are going home." and r.retried
    assert len(r.calls) == 2
    assert r.calls[0].ttft_s == 0.35  # first piece of text as the client saw it
    assert abs(r.total_s - 1.9) < 1e-9  # the whole wait, D-Bus included
    assert r.completion.draft_accepted == 7 and r.completion.texts == ["They are going home."]


def test_replies_become_the_texts() -> None:
    line = {
        "output": '{"replies": ["a", "b", "c"]}',
        "replies": ["a", "b", "c"],
        "finish": "stop",
        "retried": False,
        "warning": "",
        "calls": [_call(0.5, 2.0)],
        "client_ttft_s": 0.55,
        "client_total_s": 2.05,
    }
    assert to_result(line).completion.texts == ["a", "b", "c"]
