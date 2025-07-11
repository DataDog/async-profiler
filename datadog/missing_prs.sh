#!/usr/bin/env bash

set -euo pipefail

# === Configuration ===
UPSTREAM_REMOTE="upstream"
UPSTREAM_BRANCH="master"
ORIGIN_REMOTE="origin"
ORIGIN_BRANCH="dd/master"
GITHUB_REPO="async-profiler/async-profiler"  # format: owner/repo

# === Fetch latest from both remotes ===
echo "Fetching latest changes..."
git fetch "$UPSTREAM_REMOTE" "$UPSTREAM_BRANCH"
git fetch "$ORIGIN_REMOTE" "$ORIGIN_BRANCH"

# === Get commit hashes and full messages from upstream not in fork ===
echo "Scanning commits in $UPSTREAM_REMOTE/$UPSTREAM_BRANCH not in $ORIGIN_REMOTE/$ORIGIN_BRANCH..."
COMMITS=$(git log "${ORIGIN_REMOTE}/${ORIGIN_BRANCH}..${UPSTREAM_REMOTE}/${UPSTREAM_BRANCH}" --pretty=format:"%H%x09%B")

# === Build map of PR number -> commit hash ===
declare -A pr_to_commit

while IFS=$'\t' read -r commit_hash commit_message; do
    # Extract PR numbers from full message
    pr_numbers=$(echo "$commit_message" | grep -oE '#[0-9]+|\([0-9]+\)' | tr -d '#()' || true)

    # Skip commits with no PR reference
    [ -z "$pr_numbers" ] && continue

    for pr in $pr_numbers; do
        # Only record the first occurrence
        if [ -z "${pr_to_commit[$pr]:-}" ]; then
            pr_to_commit["$pr"]="$commit_hash"
        fi
    done
done <<< "$COMMITS"

if [ ${#pr_to_commit[@]} -eq 0 ]; then
    echo "No missing PRs found based on commit message PR references."
    exit 0
fi

# === Fetch PR details from GitHub ===
echo "Querying GitHub for PR details..."
RESULTS=()
for pr in "${!pr_to_commit[@]}"; do
    pr_data=$(gh pr view "$pr" --repo "$GITHUB_REPO" --json number,title,mergedAt,url 2>/dev/null || true)
    if [ -n "$pr_data" ] && [ "$pr_data" != "null" ]; then
        commit_hash="${pr_to_commit[$pr]}"
        # Add commit hash to JSON
        pr_data_with_hash=$(echo "$pr_data" | jq --arg hash "$commit_hash" '. + {commitHash: $hash}')
        RESULTS+=("$pr_data_with_hash")
    fi
done

# === Output sorted by mergedAt ===
if [ ${#RESULTS[@]} -eq 0 ]; then
    echo "No valid PRs found matching extracted numbers."
    exit 0
fi

echo "Missing PRs (sorted by merge date):"
printf '%s\n' "${RESULTS[@]}" | jq -s 'sort_by(.mergedAt)[] | "\(.mergedAt) \(.commitHash) \(.url): \(.title)"'
